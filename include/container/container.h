#ifndef CONTAINER_CONTAINER_H
#define CONTAINER_CONTAINER_H

#include <memory>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <initializer_list>
#include <type_traits>
#include <algorithm>
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

        friend class Container;

        explicit Iterator(NodePointer node) : current_node(node) {}

    public:
        Iterator() : current_node(nullptr) {}

        template<bool OtherIsConst, typename = std::enable_if_t<OtherIsConst || !IsConst>>
        Iterator(const Iterator<OtherIsConst>& other) noexcept :
            current_node(const_cast<NodePointer>(other.current_node)) {}

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

    void initialize_container() {
        if (sentinel_node != nullptr) {
            destroy_container_nodes();
        }

        sentinel_node = allocate_and_construct_sentinel();
        sentinel_node->next = sentinel_node;
        sentinel_node->prev = sentinel_node;

        num_elements = 0;

        if (skip_list_heads == nullptr) {
             skip_list_heads = new Node<T>*[MAX_SKIP_LEVEL];
        }

        for (int i = 0; i < MAX_SKIP_LEVEL; ++i) {
            skip_list_heads[i] = sentinel_node;
            sentinel_node->forward[i] = sentinel_node;
        }
        current_max_level = 0;

        rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        dist = std::uniform_real_distribution<double>(0.0, 1.0);
    }

    void destroy_container_nodes() noexcept {
        if (sentinel_node == nullptr) {
            if (skip_list_heads != nullptr) {
                delete[] skip_list_heads;
                skip_list_heads = nullptr;
            }
            return;
        }

        Node<T>* current = sentinel_node->next;
        while (current != sentinel_node) {
            Node<T>* next_node = current->next;
            destroy_and_deallocate_node(current);
            current = next_node;
        }
        destroy_and_deallocate_node(sentinel_node);
        sentinel_node = nullptr;

        if (skip_list_heads != nullptr) {
            delete[] skip_list_heads;
            skip_list_heads = nullptr;
        }
    }

    void copy_container_nodes_from(const Container& other) {
        for (const auto& val : other) {
            push_back(val);
        }
    }

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
            std::allocator_traits<NodeAllocator>::construct(node_allocator, new_node, true, MAX_SKIP_LEVEL);
        } catch (...) {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        return new_node;
    }

    void destroy_and_deallocate_node(Node<T>* node) {
        if (node == nullptr) return;
        std::allocator_traits<NodeAllocator>::destroy(node_allocator, node);
        node_allocator.deallocate(node, 1);
    }

    void insert_dll_node_before(Node<T>* new_node, Node<T>* position_node) {
        Node<T>* prev_node = position_node->prev;

        new_node->next = position_node;
        new_node->prev = prev_node;
        prev_node->next = new_node;
        position_node->prev = new_node;

        num_elements++;
    }

    void remove_dll_node(Node<T>* node_to_remove) {
        node_to_remove->prev->next = node_to_remove->next;
        node_to_remove->next->prev = node_to_remove->prev;
        num_elements--;
    }

    int get_random_level() const {
        int level = 0;
        while (dist(rng) < 0.5 && level < MAX_SKIP_LEVEL - 1) {
            level++;
        }
        return level;
    }

    void remove_from_skip_list(Node<T>* node_to_remove) {
        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = sentinel_node;

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < node_to_remove->value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        current = update[0]->forward[0];

        if (current != sentinel_node && current == node_to_remove) {
            for (int i = 0; i <= current->level; ++i) {
                if (update[i]->forward[i] == node_to_remove) {
                    update[i]->forward[i] = node_to_remove->forward[i];
                }
            }

            while (current_max_level > 0 && sentinel_node->forward[current_max_level] == sentinel_node) {
                current_max_level--;
            }
        }
    }

    Node<T>* find_node_in_skip_list(const value_type& value) const {
        Node<T>* current = sentinel_node;

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
    explicit Container(const Allocator& alloc = Allocator()) :
        node_allocator(alloc),
        sentinel_node(nullptr),
        num_elements(0),
        skip_list_heads(nullptr),
        current_max_level(0)
    {
        initialize_container();
    }

    Container(size_type count, const value_type& value, const Allocator& alloc = Allocator()) :
        Container(alloc)
    {
        for (size_type i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    Container(size_type count, const Allocator& alloc = Allocator()) :
        Container(alloc)
    {
        for (size_type i = 0; i < count; ++i) {
            push_back(T());
        }
    }

    template <typename InputIt,
              typename = std::enable_if_t<
                  std::is_base_of<std::input_iterator_tag,
                                  typename std::iterator_traits<InputIt>::iterator_category>::value
                  && !std::is_same_v<std::remove_reference_t<InputIt>, size_type>
              >>
    Container(InputIt first, InputIt last, const Allocator& alloc = Allocator()) :
        Container(alloc)
    {
        for (; first != last; ++first) {
            push_back(*first);
        }
    }

    Container(std::initializer_list<value_type> init, const Allocator& alloc = Allocator()) :
        Container(alloc)
    {
        for (const auto& val : init) {
            push_back(val);
        }
    }

    Container(const Container& other) :
        node_allocator(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator())),
        sentinel_node(nullptr),
        num_elements(0),
        skip_list_heads(nullptr),
        current_max_level(0)
    {
        initialize_container();
        copy_container_nodes_from(other);
    }

    Container(const Container& other, const Allocator& alloc) :
        node_allocator(alloc),
        sentinel_node(nullptr),
        num_elements(0),
        skip_list_heads(nullptr),
        current_max_level(0)
    {
        initialize_container();
        copy_container_nodes_from(other);
    }

    Container(Container&& other) noexcept :
        node_allocator(std::move(other.node_allocator)),
        sentinel_node(other.sentinel_node),
        num_elements(other.num_elements),
        skip_list_heads(other.skip_list_heads),
        current_max_level(other.current_max_level),
        rng(std::move(other.rng)),
        dist(std::move(other.dist))
    {
        other.sentinel_node = nullptr;
        other.num_elements = 0;
        other.skip_list_heads = nullptr;
        other.current_max_level = 0;
    }

    Container(Container&& other, const Allocator& alloc) :
        node_allocator(alloc),
        sentinel_node(nullptr),
        num_elements(0),
        skip_list_heads(nullptr),
        current_max_level(0)
    {
        if (node_allocator == other.node_allocator) {
            sentinel_node = other.sentinel_node;
            num_elements = other.num_elements;
            skip_list_heads = other.skip_list_heads;
            current_max_level = other.current_max_level;
            rng = std::move(other.rng);
            dist = std::move(other.dist);

            other.sentinel_node = nullptr;
            other.skip_list_heads = nullptr;
            other.num_elements = 0;
            other.current_max_level = 0;
        } else {
            initialize_container();
            copy_container_nodes_from(other);
            other.clear();
        }
    }

    ~Container() {
        destroy_container_nodes();
    }

    Container& operator=(const Container& other) {
        if (this != &other) {
            if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
                if (node_allocator != other.node_allocator) {
                    destroy_container_nodes();
                    node_allocator = other.node_allocator;
                    initialize_container();
                } else {
                    this->clear();
                }
            } else {
                this->clear();
            }
            copy_container_nodes_from(other);
        }
        return *this;
    }

    Container& operator=(Container&& other) noexcept {
        if (this != &other) {
            destroy_container_nodes();

            if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                node_allocator = std::move(other.node_allocator);
                sentinel_node = other.sentinel_node;
                num_elements = other.num_elements;
                skip_list_heads = other.skip_list_heads;
                current_max_level = other.current_max_level;
                rng = std::move(other.rng);
                dist = std::move(other.dist);

                other.sentinel_node = nullptr;
                other.num_elements = 0;
                other.skip_list_heads = nullptr;
                other.current_max_level = 0;
            } else if (node_allocator == other.node_allocator) {
                sentinel_node = other.sentinel_node;
                num_elements = other.num_elements;
                skip_list_heads = other.skip_list_heads;
                current_max_level = other.current_max_level;
                rng = std::move(other.rng);
                dist = std::move(other.dist);

                other.sentinel_node = nullptr;
                other.num_elements = 0;
                other.skip_list_heads = nullptr;
                other.current_max_level = 0;
            } else {
                copy_container_nodes_from(other);
                other.clear();
            }
        }
        return *this;
    }

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

    bool empty() const noexcept {
        return num_elements == 0;
    }

    size_type size() const noexcept {
        return num_elements;
    }

    size_type max_size() const noexcept {
        return std::allocator_traits<NodeAllocator>::max_size(node_allocator);
    }

    void clear() noexcept {
        destroy_container_nodes();
        initialize_container();
    }

    iterator insert([[maybe_unused]] const_iterator pos, const value_type& value) {
        int node_level = get_random_level();
        Node<T>* new_node = allocate_and_construct_node(value, node_level);

        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = sentinel_node;

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        Node<T>* next_dll_node = current->next;
        insert_dll_node_before(new_node, next_dll_node);

        if (new_node->level > current_max_level) {
            for (int i = current_max_level + 1; i <= new_node->level; ++i) {
                update[i] = sentinel_node;
            }
            current_max_level = new_node->level;
        }

        for (int i = 0; i <= new_node->level; ++i) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }

        return iterator(new_node);
    }

    iterator insert([[maybe_unused]] const_iterator pos, value_type&& value) {
        int node_level = get_random_level();
        Node<T>* new_node = allocate_and_construct_node(std::move(value), node_level);

        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = sentinel_node;

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < new_node->value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        Node<T>* next_dll_node = current->next;
        insert_dll_node_before(new_node, next_dll_node);

        if (new_node->level > current_max_level) {
            for (int i = current_max_level + 1; i <= new_node->level; ++i) {
                update[i] = sentinel_node;
            }
            current_max_level = new_node->level;
        }

        for (int i = 0; i <= new_node->level; ++i) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }

        return iterator(new_node);
    }

    iterator insert([[maybe_unused]] const_iterator pos, size_type count, const value_type& value) {
        if (count == 0) {
            return iterator(const_cast<Node<T>*>(pos.current_node));
        }

        iterator first_inserted_it = end();
        bool first_set = false;

        for (size_type i = 0; i < count; ++i) {
            iterator current_it = insert(end(), value);
            if (!first_set || (current_it != end() && (*current_it < *first_inserted_it))) {
                first_inserted_it = current_it;
                first_set = true;
            }
        }
        return first_inserted_it;
    }

    template <typename InputIt,
              typename = std::enable_if_t<
                  std::is_base_of<std::input_iterator_tag,
                                  typename std::iterator_traits<InputIt>::iterator_category>::value
                  && !std::is_same_v<std::remove_reference_t<InputIt>, size_type>
              >>
    iterator insert([[maybe_unused]] const_iterator pos, InputIt first, InputIt last) {
        iterator first_inserted_it = end();
        bool first_set = false;

        for (auto it = first; it != last; ++it) {
            iterator current_it = insert(end(), *it);
            if (!first_set || (current_it != end() && (*current_it < *first_inserted_it))) {
                first_inserted_it = current_it;
                first_set = true;
            }
        }
        return first_inserted_it;
    }

    iterator insert([[maybe_unused]] const_iterator pos, std::initializer_list<value_type> ilist) {
        return insert(end(), ilist.begin(), ilist.end());
    }

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

    iterator erase(const_iterator first, const_iterator last) {
        while (first != last) {
            first = erase(first);
        }
        return iterator(const_cast<Node<T>*>(last.current_node));
    }

    void push_front(const value_type& value) {
        insert(end(), value);
    }

    void push_front(value_type&& value) {
        insert(end(), std::move(value));
    }

    void pop_front() {
        if (empty()) {
            throw std::out_of_range("pop_front() called on empty container.");
        }
        erase(begin());
    }

    void push_back(const value_type& value) {
        insert(end(), value);
    }

    void push_back(value_type&& value) {
        insert(end(), std::move(value));
    }

    void pop_back() {
        if (empty()) {
            throw std::out_of_range("pop_back() called on empty container.");
        }
        erase(--end());
    }

    void swap(Container& other) noexcept {
        using std::swap;
        if (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            swap(node_allocator, other.node_allocator);
        }
        swap(sentinel_node, other.sentinel_node);
        swap(num_elements, other.num_elements);
        swap(skip_list_heads, other.skip_list_heads);
        swap(current_max_level, other.current_max_level);
        swap(rng, other.rng);
        swap(dist, other.dist);
    }

    bool contains(const value_type& value) const {
        return find_node_in_skip_list(value) != nullptr;
    }

    iterator find(const value_type& value) {
        Node<T>* node = find_node_in_skip_list(value);
        return (node != nullptr) ? iterator(node) : end();
    }

    const_iterator find(const value_type& value) const {
        Node<T>* node = find_node_in_skip_list(value);
        return (node != nullptr) ? const_iterator(node) : cend();
    }
};

template <typename T, typename Alloc>
void swap(Container<T, Alloc>& a, Container<T, Alloc>& b) noexcept {
    a.swap(b);
}

#endif // CONTAINER_CONTAINER_H