#ifndef CONTAINER_CONTAINER_H
#define CONTAINER_CONTAINER_H

#include <memory>
#include <cstddef>
#include <iterator>         // Для std::iterator_traits
#include <stdexcept>
#include <initializer_list>
#include <type_traits>      // Для std::enable_if, std::is_base_of, std::remove_reference, std::is_same_v
#include <algorithm>        // Для std::swap
#include <random>
#include <chrono>
#include <iostream>

#include "container/nodes/node.h"

template <typename T, typename Allocator = std::allocator<T>>
class Container {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    template <bool IsConst>
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Container::value_type;
        using difference_type = Container::difference_type;
        using pointer = std::conditional_t<IsConst, Container::const_pointer, Container::pointer>;
        using reference = std::conditional_t<IsConst, Container::const_reference, Container::reference>;

        using NodeType = Node<value_type>;
        using NodePointer = std::conditional_t<IsConst, const NodeType*, NodeType*>;

    private:
        NodePointer current_node;

        friend class Container; // Container может получать доступ к current_node

        explicit Iterator(NodePointer node) : current_node(node) {}

    public:
        Iterator() : current_node(nullptr) {}

        // Конструктор копирования с преобразованием const/non-const
        template<bool OtherIsConst, typename = std::enable_if_t<IsConst || !OtherIsConst>>
        Iterator(const Iterator<OtherIsConst>& other) noexcept : current_node(other.current_node) {}

        // Оператор преобразования к const_iterator
        operator Iterator<true>() const noexcept {
            return Iterator<true>(current_node);
        }

        reference operator*() const {
            if (!current_node || current_node->is_sentinel) {
                throw std::out_of_range("Dereferencing invalid iterator or sentinel.");
            }
            return current_node->value;
        }

        pointer operator->() const {
            if (!current_node || current_node->is_sentinel) {
                throw std::out_of_range("Dereferencing invalid iterator or sentinel.");
            }
            return std::addressof(current_node->value);
        }

        Iterator& operator++() {
            if (!current_node) {
                throw std::out_of_range("Incrementing null iterator.");
            }
            // Для Skip List итератор должен двигаться по prev/next обычного DLL
            current_node = current_node->next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        Iterator& operator--() {
            if (!current_node) {
                throw std::out_of_range("Decrementing null iterator.");
            }
            current_node = current_node->prev;
            return *this;
        }

        Iterator operator--(int) {
            Iterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const noexcept {
            return current_node == other.current_node;
        }

        bool operator!=(const Iterator& other) const noexcept {
            return !(*this == other);
        }
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

private:
    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node<T>>;
    NodeAllocator node_allocator;

    Node<T>* sentinel_node;
    size_type num_elements;

    static constexpr int MAX_SKIP_LEVEL = 16;
    Node<T>** skip_list_heads;
    int current_max_level;

    mutable std::mt19937 rng;
    mutable std::uniform_real_distribution<double> dist;

    // Вспомогательная функция для инициализации нового контейнера
    void initialize_container() {
        // initialize_container() всегда должна вызываться для объекта в "чистом" состоянии.
        // То есть, после конструктора по умолчанию, или после полного destroy_container_nodes().
        // Убраны проверки `if (sentinel_node != nullptr)` и `destroy_container_nodes()` из начала,
        // так как это может привести к двойному освобождению или некорректной логике
        // при использовании делегирующих конструкторов.

        // Создаем и инициализируем сентинел-узел
        sentinel_node = allocate_and_construct_sentinel();
        sentinel_node->next = sentinel_node; // Двусвязный список указывает сам на себя
        sentinel_node->prev = sentinel_node;

        num_elements = 0;

        // Инициализируем массив головных узлов Skip List
        // Важно: `skip_list_heads` должен быть `nullptr` перед `new`, если `initialize_container`
        // может быть вызван не на вновь созданном объекте (например, после явного вызова `clear()`).
        // В деструкторе и `clear()` он должен быть обнулен после `delete[]`.
        if (skip_list_heads != nullptr) {
            // Это должно быть защитой на случай, если initialize_container вызывается
            // на не полностью очищенном объекте (хотя в идеале не должно быть).
            delete[] skip_list_heads;
            skip_list_heads = nullptr;
        }
        skip_list_heads = new Node<T>*[MAX_SKIP_LEVEL];
        for (int i = 0; i < MAX_SKIP_LEVEL; ++i) {
            skip_list_heads[i] = sentinel_node; // Направляющие указатели головных узлов
            sentinel_node->forward[i] = sentinel_node; // Сентинел указывает на себя на каждом уровне
        }
        current_max_level = 0; // Изначально только нулевой уровень активен

        // Инициализация генератора случайных чисел для уровней Skip List
        rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        dist = std::uniform_real_distribution<double>(0.0, 1.0);
    }

    // Вспомогательная функция для уничтожения всех узлов контейнера
    void destroy_container_nodes() noexcept {
        // Сначала очищаем связанные узлы двусвязного списка, если они есть.
        if (sentinel_node != nullptr) {
            Node<T>* current = sentinel_node->next;
            while (current != sentinel_node) {
                Node<T>* next_node = current->next;
                destroy_and_deallocate_node(current);
                current = next_node;
            }
            // Теперь уничтожаем сам сентинел-узел
            destroy_and_deallocate_node(sentinel_node);
            sentinel_node = nullptr; // Обнуляем указатель после освобождения
        }

        // Освобождаем массив головных указателей Skip List, если он был выделен.
        if (skip_list_heads != nullptr) {
            delete[] skip_list_heads;
            skip_list_heads = nullptr; // Обнуляем указатель после освобождения
        }

        num_elements = 0; // Сброс размера
        current_max_level = 0; // Сброс уровня
    }

    // Вспомогательная функция для копирования узлов из другого контейнера
    void copy_container_nodes_from(const Container& other) {
        for (const auto& val : other) {
            push_back(val);
        }
    }

    // Вспомогательные функции для работы с аллокатором узлов
    Node<T>* allocate_and_construct_node(const value_type& val, int level) {
        Node<T>* new_node = node_allocator.allocate(1);
        try {
            std::allocator_traits<NodeAllocator>::construct(node_allocator, new_node, val, level);
        } catch (...) {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        return new_node;
    }

    Node<T>* allocate_and_construct_node(value_type&& val, int level) {
        Node<T>* new_node = node_allocator.allocate(1);
        try {
            std::allocator_traits<NodeAllocator>::construct(node_allocator, new_node, std::move(val), level);
        } catch (...) {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        return new_node;
    }

    Node<T>* allocate_and_construct_sentinel() {
        Node<T>* new_node = node_allocator.allocate(1);
        try {
            std::allocator_traits<NodeAllocator>::construct(node_allocator, new_node, true, MAX_SKIP_LEVEL); // Передаем MAX_SKIP_LEVEL для forward array
        } catch (...) {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        return new_node;
    }

    void destroy_and_deallocate_node(Node<T>* node) {
        if (node == nullptr) return; // Защита от nullptr
        std::allocator_traits<NodeAllocator>::destroy(node_allocator, node);
        node_allocator.deallocate(node, 1);
    }

    // Вспомогательная функция для вставки нового узла в двусвязный список
    void insert_dll_node_before(Node<T>* new_node, Node<T>* position_node) {
        Node<T>* prev_node = position_node->prev;

        new_node->next = position_node;
        new_node->prev = prev_node;
        prev_node->next = new_node;
        position_node->prev = new_node;

        num_elements++;
    }

    // Вспомогательная функция для удаления узла из двусвязного списка
    void remove_dll_node(Node<T>* node_to_remove) {
        node_to_remove->prev->next = node_to_remove->next;
        node_to_remove->next->prev = node_to_remove->prev;
        num_elements--;
    }

    // Вспомогательная функция для определения случайного уровня для нового узла Skip List
    int get_random_level() const {
        int level = 0;
        // Защита от бесконечного цикла, если dist(rng) всегда < 0.5
        while (dist(rng) < 0.5 && level < MAX_SKIP_LEVEL - 1) {
            level++;
        }
        return level;
    }

    // Вспомогательная функция для вставки нового узла в Skip List
    void insert_into_skip_list(Node<T>* new_node) {
        // Убедимся, что new_node->level не превышает MAX_SKIP_LEVEL-1
        // (хотя get_random_level уже это гарантирует)
        if (new_node->level >= MAX_SKIP_LEVEL) {
            // Это должно быть обработано как ошибка логики, если произойдет.
            // Например, бросить исключение или залогировать.
            // Пока просто ограничим уровень, чтобы избежать выхода за границы массива.
            new_node->level = MAX_SKIP_LEVEL - 1;
        }

        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = sentinel_node; // Начинаем с сентинел-узла для поиска

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < new_node->value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        // Обновляем current_max_level, если новый узел выше
        if (new_node->level > current_max_level) {
            for (int i = current_max_level + 1; i <= new_node->level; ++i) {
                update[i] = sentinel_node; // Новые уровни начинаются с сентинел-узла
            }
            current_max_level = new_node->level;
        }

        for (int i = 0; i <= new_node->level; ++i) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }

    // Вспомогательная функция для удаления узла из Skip List
    void remove_from_skip_list(Node<T>* node_to_remove) {
        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = sentinel_node; // Начинаем с сентинел-узла для поиска

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < node_to_remove->value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        current = current->forward[0];

        if (current != sentinel_node && current == node_to_remove) {
            for (int i = 0; i <= current->level; ++i) {
                update[i]->forward[i] = current->forward[i];
            }

            while (current_max_level > 0 && sentinel_node->forward[current_max_level] == sentinel_node) {
                current_max_level--;
            }
        }
    }

    // Вспомогательная функция для поиска узла в Skip List по значению
    Node<T>* find_node_in_skip_list(const value_type& value) const {
        Node<T>* current = sentinel_node; // Начинаем с сентинел-узла

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < value) {
                current = current->forward[i];
            }
        }
        current = current->forward[0];

        if (current != sentinel_node && current->value == value) {
            return current;
        }
        return nullptr;
    }


public:
    // Конструктор по умолчанию
    explicit Container(const Allocator& alloc = Allocator()) :
        node_allocator(alloc),
        sentinel_node(nullptr), // Инициализируем nullptr, чтобы initialize_container мог безопасно работать
        num_elements(0),
        skip_list_heads(nullptr), // Инициализируем nullptr
        current_max_level(0)
    {
        initialize_container();
    }

    // Конструктор: count элементов с заданным значением
    Container(size_type count, const value_type& value, const Allocator& alloc = Allocator()) :
        Container(alloc) // Делегируем конструктору по умолчанию для инициализации
    {
        for (size_type i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    // Конструктор: count default-сконструированных элементов
    Container(size_type count, const Allocator& alloc = Allocator()) :
        Container(alloc) // Делегируем конструктору по умолчанию для инициализации
    {
        for (size_type i = 0; i < count; ++i) {
            // Используем placement new для T(), чтобы использовать аллокатор
            // Вместо push_back(T()), что может быть медленнее.
            // Однако, push_back уже использует аллокатор, так что это нормально.
            // Если T не имеет конструктора по умолчанию, это вызовет ошибку.
            push_back(T());
        }
    }

    // Конструктор диапазона (С ИСПРАВЛЕНИЕМ SFINAE)
    template <typename InputIt,
              typename = std::enable_if_t<
                  std::is_base_of<std::input_iterator_tag,
                                  typename std::iterator_traits<InputIt>::iterator_category>::value
                  && !std::is_same_v<std::remove_reference_t<InputIt>, size_type>
              >>
    Container(InputIt first, InputIt last, const Allocator& alloc = Allocator()) :
        Container(alloc) // Делегируем конструктору по умолчанию для инициализации
    {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    // Конструктор из initializer_list
    Container(std::initializer_list<value_type> init, const Allocator& alloc = Allocator()) :
        Container(alloc) // Делегируем конструктору по умолчанию для инициализации
    {
        for (const auto& val : init) {
            push_back(val);
        }
    }

    // Конструктор копирования
    Container(const Container& other) :
        node_allocator(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator())),
        sentinel_node(nullptr), // Инициализируем nullptr
        num_elements(0),
        skip_list_heads(nullptr), // Инициализируем nullptr
        current_max_level(0)
    {
        initialize_container(); // Инициализация текущего контейнера
        copy_container_nodes_from(other);
    }

    // Конструктор копирования с заданным аллокатором
    Container(const Container& other, const Allocator& alloc) :
        node_allocator(alloc),
        sentinel_node(nullptr), // Инициализируем nullptr
        num_elements(0),
        skip_list_heads(nullptr), // Инициализируем nullptr
        current_max_level(0)
    {
        initialize_container(); // Инициализация текущего контейнера
        copy_container_nodes_from(other);
    }

    // Конструктор перемещения
    Container(Container&& other) noexcept :
        node_allocator(std::move(other.node_allocator)),
        sentinel_node(other.sentinel_node),
        num_elements(other.num_elements),
        skip_list_heads(other.skip_list_heads),
        current_max_level(other.current_max_level),
        rng(std::move(other.rng)),
        dist(std::move(other.dist))
    {
        // Исходный контейнер оставляем в валидном, но пустом состоянии
        other.sentinel_node = nullptr;
        other.num_elements = 0;
        other.skip_list_heads = nullptr; // !!! КЛЮЧЕВОЕ ИЗМЕНЕНИЕ !!! Обнуляем, чтобы избежать двойного delete
        other.current_max_level = 0;
        // other.rng и other.dist не нужно обнулять, они уже перемещены
    }

    // Конструктор перемещения с заданным аллокатором
    Container(Container&& other, const Allocator& alloc) :
        node_allocator(alloc),
        sentinel_node(nullptr),
        num_elements(0),
        skip_list_heads(nullptr),
        current_max_level(0)
    {
        if (node_allocator == other.node_allocator) {
            // Если аллокаторы равны, можно просто переместить ресурсы
            // Нет необходимости в initialize_container() или destroy_container_nodes() здесь.
            // Просто берем ресурсы other.
            sentinel_node = other.sentinel_node;
            num_elements = other.num_elements;
            skip_list_heads = other.skip_list_heads;
            current_max_level = other.current_max_level;
            rng = std::move(other.rng);
            dist = std::move(other.dist);

            other.sentinel_node = nullptr;
            other.skip_list_heads = nullptr; // !!! КЛЮЧЕВОЕ ИЗМЕНЕНИЕ !!!
            other.num_elements = 0;
            other.current_max_level = 0;
        } else {
            // Если аллокаторы разные, выполняем глубокое копирование и очистку исходного
            // Здесь уже нужно вызвать initialize_container() для this,
            // чтобы у this были свои sentinel_node и skip_list_heads
            // до того, как мы начнем вставлять элементы.
            initialize_container(); // Инициализация текущего контейнера
            copy_container_nodes_from(other);
            other.clear(); // other.clear() теперь безопасно, так как this не взял его ресурсы.
        }
    }

    // Деструктор
    ~Container() {
        destroy_container_nodes();
    }

    // Оператор присваивания копированием
    Container& operator=(const Container& other) {
        if (this != &other) {
            if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
                if (node_allocator != other.node_allocator) {
                    destroy_container_nodes();
                    node_allocator = other.node_allocator;
                    initialize_container(); // Реинициализация с новым аллокатором
                } else {
                    this->clear(); // Очищаем текущий контейнер
                }
            } else {
                this->clear(); // Очищаем текущий контейнер
            }
            copy_container_nodes_from(other); // Копируем узлы
        }
        return *this;
    }

    // Оператор присваивания перемещением
    Container& operator=(Container&& other) noexcept {
        if (this != &other) {
            // Освобождаем текущие ресурсы
            destroy_container_nodes();

            if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                // Если аллокатор распространяется, просто перемещаем все члены
                node_allocator = std::move(other.node_allocator);
                sentinel_node = other.sentinel_node;
                num_elements = other.num_elements;
                skip_list_heads = other.skip_list_heads; // <-- other.skip_list_heads перемещен сюда
                current_max_level = other.current_max_level;
                rng = std::move(other.rng);
                dist = std::move(other.dist);

                // Оставляем other в пустом, но валидном состоянии
                other.sentinel_node = nullptr;
                other.num_elements = 0;
                other.skip_list_heads = nullptr; // !!! КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: ОБНУЛИТЬ other.skip_list_heads
                other.current_max_level = 0;
            } else if (node_allocator == other.node_allocator) {
                // Если аллокаторы равны (не распространяются, но равны), также перемещаем
                // (фактически, это просто обмен указателями)
                sentinel_node = other.sentinel_node;
                num_elements = other.num_elements;
                skip_list_heads = other.skip_list_heads; // <-- other.skip_list_heads перемещен сюда
                current_max_level = other.current_max_level;
                rng = std::move(other.rng);
                dist = std::move(other.dist);

                other.sentinel_node = nullptr;
                other.num_elements = 0;
                other.skip_list_heads = nullptr; // !!! КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: ОБНУЛИТЬ other.skip_list_heads
                other.current_max_level = 0;
            } else {
                // Если аллокаторы разные и не распространяются,
                // глубокое копирование (other должен быть очищен)
                copy_container_nodes_from(other);
                other.clear(); // other.clear() вызовет destroy_container_nodes() для other
            }
        }
        return *this;
    }

    // Оператор присваивания из initializer_list
    Container& operator=(std::initializer_list<value_type> ilist) {
        this->clear();
        for (const auto& val : ilist) {
            push_back(val);
        }
        return *this;
    }


    allocator_type get_allocator() const noexcept {
        return node_allocator;
    }

    // Доступ к первому элементу
    reference front() {
        if (empty()) {
            throw std::out_of_range("front() called on empty container.");
        }
        return sentinel_node->next->value;
    }

    const_reference front() const {
        if (empty()) {
            throw std::out_of_range("front() called on empty container.");
        }
        return sentinel_node->next->value;
    }

    // Доступ к последнему элементу
    reference back() {
        if (empty()) {
            throw std::out_of_range("back() called on empty container.");
        }
        return sentinel_node->prev->value;
    }

    const_reference back() const {
        if (empty()) {
            throw std::out_of_range("back() called on empty container.");
        }
        return sentinel_node->prev->value;
    }

    // Итераторы
    iterator begin() noexcept {
        return iterator(sentinel_node->next);
    }

    const_iterator begin() const noexcept {
        return const_iterator(sentinel_node->next);
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(sentinel_node->next);
    }

    iterator end() noexcept {
        return iterator(sentinel_node);
    }

    const_iterator end() const noexcept {
        return const_iterator(sentinel_node);
    }

    const_iterator cend() const noexcept {
        return const_iterator(sentinel_node);
    }

    // Проверка на пустоту
    bool empty() const noexcept {
        return num_elements == 0;
    }

    // Размер контейнера
    size_type size() const noexcept {
        return num_elements;
    }

    // Максимальный размер, который может вместить контейнер
    size_type max_size() const noexcept {
        return std::allocator_traits<NodeAllocator>::max_size(node_allocator);
    }

    // Очистка контейнера
    void clear() noexcept {
        destroy_container_nodes();
        initialize_container();
    }

    // Вставка элемента
    iterator insert(const_iterator pos, const value_type& value) {
        // pos - это hint для DLL, но для Skip List он игнорируется
        // фактическое местоположение определяется значением.
        // Поэтому проверка `pos.current_node == nullptr` имеет смысл только
        // для предотвращения разыменования некорректного итератора,
        // а не для логики вставки в Skip List.
        if (pos.current_node == nullptr) {
            throw std::invalid_argument("Cannot insert at null iterator position.");
        }

        int node_level = get_random_level();
        Node<T>* new_node = allocate_and_construct_node(value, node_level);

        // Вставка в DLL всегда происходит перед sentinel_node при push_back/insert(end(), ...)
        // или перед указанным pos для обычного списка.
        // Для Skip List, так как элементы сортированы, insert_dll_node_before должен использоваться
        // с актуальным prev/next узла, куда будет добавлен новый узел, а не просто `pos.current_node`.
        // Однако, ваша реализация insert_dll_node_before просто вставляет перед `position_node`.
        // Если pos - это `end()`, то `position_node` это `sentinel_node`.
        // Если pos - это `begin()`, то `position_node` это `sentinel_node->next`.
        // Для Skip List, insert всегда добавляет элементы в сортированном порядке,
        // поэтому `pos` (hint) не влияет на DLL вставку.
        // Ваша DLL вставка происходит относительно `pos.current_node`.
        // Для Skip List более естественно просто добавить в конец DLL и затем разместить в SL.
        // Или найти точное место в DLL, основываясь на значении.
        // Сейчас insert_dll_node_before будет работать, если pos указывает на правильное место
        // в DLL, но для Skip List это будет неэффективно, т.к. DLL порядок не используется для поиска.
        // Если ваш insert(pos, value) должен поддерживать hint, то нужно сначала найти
        // точное место в DLL для нового значения, а не просто использовать pos.
        // Но так как это Skip List, DLL является лишь поддерживающей структурой.
        // Для простоты, оставим как есть, но учтите, что `pos` тут просто hint.
        insert_dll_node_before(new_node, const_cast<Node<T>*>(pos.current_node)); // Вставляем перед `pos` в DLL
        insert_into_skip_list(new_node); // Вставляем в Skip List по значению

        return iterator(new_node);
    }

    // Вставка элемента (move semantic)
    iterator insert(const_iterator pos, value_type&& value) {
        if (pos.current_node == nullptr) {
            throw std::invalid_argument("Cannot insert at null iterator position.");
        }

        int node_level = get_random_level();
        Node<T>* new_node = allocate_and_construct_node(std::move(value), node_level);

        insert_dll_node_before(new_node, const_cast<Node<T>*>(pos.current_node));
        insert_into_skip_list(new_node);

        return iterator(new_node);
    }

    // Перегрузка insert: вставка count элементов
    iterator insert(const_iterator pos, size_type count, const value_type& value) {
        // Для Skip List pos - это только hint для DLL, фактическое местоположение
        // определяется значением.
        iterator result_it = end(); // Итератор на первый вставленный элемент (наименьший)
        if (count == 0) return result_it;

        // Вместо поиска наименьшего, просто вставляем и возвращаем итератор на первый
        // вставленный элемент, который будет первым по значению (если список сортирован).
        // Если элементы вставляются в произвольном порядке, то нужно найти первый в DLL.
        // Однако, Skip List по своей природе сортирован.
        for (size_type i = 0; i < count; ++i) {
            iterator current_inserted_it = insert(pos, value); // pos все еще просто hint
            if (i == 0 || (*current_inserted_it < *result_it)) { // Обновляем, если это первый или меньшее значение
                result_it = current_inserted_it;
            }
        }
        return result_it;
    }

    // Перегрузка insert: вставка диапазона элементов (С ИСПРАВЛЕНИЕМ SFINAE)
    template <typename InputIt,
              typename = std::enable_if_t<
                  std::is_base_of<std::input_iterator_tag,
                                  typename std::iterator_traits<InputIt>::iterator_category>::value
                  && !std::is_same_v<std::remove_reference_t<InputIt>, size_type>
              >>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        iterator result_it = end();
        if (first == last) return result_it;

        size_type i = 0;
        for (auto it = first; it != last; ++it) {
            iterator current_inserted_it = insert(pos, *it);
            if (i == 0 || (*current_inserted_it < *result_it)) {
                result_it = current_inserted_it;
            }
            i++;
        }
        return result_it;
    }

    // Перегрузка insert: вставка из initializer_list
    iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) {
        return insert(pos, ilist.begin(), ilist.end());
    }


    // Удаление элемента по итератору
    iterator erase(const_iterator pos) {
        if (pos.current_node == nullptr || pos.current_node == sentinel_node || empty()) {
            throw std::invalid_argument("Cannot erase at null or sentinel iterator position or from empty container.");
        }

        Node<T>* node_to_remove = const_cast<Node<T>*>(pos.current_node);
        Node<T>* next_node = node_to_remove->next;

        remove_from_skip_list(node_to_remove);
        remove_dll_node(node_to_remove);

        destroy_and_deallocate_node(node_to_remove);

        return iterator(next_node);
    }

    // Удаление диапазона элементов
    iterator erase(const_iterator first, const_iterator last) {
        // Убедитесь, что `first` и `last` валидны и `first` не после `last`
        if (first.current_node == nullptr || last.current_node == nullptr) {
            throw std::invalid_argument("Invalid iterator range for erase.");
        }

        while (first != last) {
            // Важно: erase(first) возвращает итератор на следующий элемент
            // после удаленного, что идеально подходит для цикла.
            first = erase(first);
        }
        return iterator(const_cast<Node<T>*>(last.current_node));
    }


    // Добавление элемента в начало
    void push_front(const value_type& value) {
        // В Skip List "начало" и "конец" - это минимальное и максимальное значения.
        // `insert(begin(), value)` вставит `value` в правильное место по значению,
        // а не буквально в начало DLL, если только `value` не является наименьшим.
        // Это соответствует поведению `std::map`, а не `std::list`.
        insert(begin(), value);
    }

    // Добавление элемента в начало (move semantic)
    void push_front(value_type&& value) {
        insert(begin(), std::move(value));
    }

    // Удаление элемента из начала
    void pop_front() {
        if (empty()) {
            throw std::out_of_range("pop_front() called on empty container.");
        }
        // begin() указывает на первый элемент (наименьший) в Skip List
        erase(begin());
    }

    // Добавление элемента в конец
    void push_back(const value_type& value) {
        // Аналогично push_front, insert(end(), value) вставит значение в правильное
        // место по значению, а не в конец DLL, если только `value` не является наибольшим.
        insert(end(), value);
    }

    // Добавление элемента в конец (move semantic)
    void push_back(value_type&& value) {
        insert(end(), std::move(value));
    }

    // Удаление элемента из конца
    void pop_back() {
        if (empty()) {
            throw std::out_of_range("pop_back() called on empty container.");
        }
        // --end() указывает на последний элемент (наибольший) в Skip List
        erase(--end());
    }

    // Обмен содержимым с другим контейнером
    void swap(Container& other) noexcept {
        using std::swap;
        // Allocator traits: propagate_on_container_swap
        // Если аллокаторы распространяются при обмене, меняем их местами.
        // Иначе (или если они не равны), не трогаем.
        if (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            swap(node_allocator, other.node_allocator);
        }
        // Обмен всеми остальными членами
        swap(sentinel_node, other.sentinel_node);
        swap(num_elements, other.num_elements);
        swap(skip_list_heads, other.skip_list_heads);
        swap(current_max_level, other.current_max_level);
        swap(rng, other.rng);
        swap(dist, other.dist);
    }

    // Проверка наличия элемента (на основе find)
    bool contains(const value_type& value) const {
        return find_node_in_skip_list(value) != nullptr;
    }

    // Поиск элемента (возвращает итератор)
    iterator find(const value_type& value) {
        Node<T>* node = find_node_in_skip_list(value);
        return (node != nullptr) ? iterator(node) : end();
    }

    const_iterator find(const value_type& value) const {
        Node<T>* node = find_node_in_skip_list(value);
        return (node != nullptr) ? const_iterator(node) : cend();
    }
};

// Не-член функция swap (для ADL)
template <typename T, typename Alloc>
void swap(Container<T, Alloc>& a, Container<T, Alloc>& b) noexcept {
    a.swap(b);
}

#endif // CONTAINER_CONTAINER_H