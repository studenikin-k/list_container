#ifndef CONTAINER_CONTAINER_H
#define CONTAINER_CONTAINER_H

#include <memory>       // For std::allocator, std::addressof
#include <cstddef>      // For std::ptrdiff_t, std::size_t
#include <iterator>     // For std::bidirectional_iterator_tag
#include <stdexcept>    // For exceptions
#include <initializer_list> // For std::initializer_list
#include <type_traits>  // For std::conditional_t, std::enable_if_t
#include <algorithm>    // For std::swap
#include <random>       // For random level generation (std::mt19937, std::uniform_real_distribution)
#include <chrono>       // For seeding random number generator (std::chrono::high_resolution_clock)
#include <iostream>     // Potentially for debugging or if print_container is moved here, but generally not needed for core container

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

        template<bool OtherIsConst, typename = std::enable_if_t<IsConst || !OtherIsConst>>
        Iterator(const Iterator<OtherIsConst>& other) noexcept : current_node(other.current_node) {}

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
        sentinel_node = allocate_and_construct_sentinel();
        num_elements = 0;

        sentinel_node->forward.assign(MAX_SKIP_LEVEL, sentinel_node);
        skip_list_heads = new Node<T>*[MAX_SKIP_LEVEL];
        for (int i = 0; i < MAX_SKIP_LEVEL; ++i) {
            skip_list_heads[i] = sentinel_node;
        }
        current_max_level = 0;

        rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        dist = std::uniform_real_distribution<double>(0.0, 1.0);
    }

    void destroy_container_nodes() {
        if (sentinel_node == nullptr) { // Check if sentinel_node is valid before attempting to destroy
            if (skip_list_heads != nullptr) { // If sentinel is null, but heads exist, implies invalid state. Clean up.
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

        delete[] skip_list_heads; // This should always be safe as it's either nullptr or valid
        skip_list_heads = nullptr;
    }

    void copy_container_nodes_from(const Container& other) {
        for (const auto& val : other) {
            push_back(val);
        }
    }

    void transfer_container_nodes_from(Container&& other) noexcept {
        if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
            destroy_container_nodes();
            node_allocator = std::move(other.node_allocator);
            sentinel_node = other.sentinel_node;
            num_elements = other.num_elements;
            skip_list_heads = other.skip_list_heads;
            current_max_level = other.current_max_level;
            rng = std::move(other.rng);
            dist = std::move(other.dist);

            // Re-initialize 'other' to a valid empty state
            other.initialize_container(); // <-- ИСПРАВЛЕНИЕ 1: Инициализируем other
        } else if (node_allocator == other.node_allocator) {
            destroy_container_nodes();
            sentinel_node = other.sentinel_node;
            num_elements = other.num_elements;
            skip_list_heads = other.skip_list_heads;
            current_max_level = other.current_max_level;
            rng = std::move(other.rng);
            dist = std::move(other.dist);

            // Re-initialize 'other' to a valid empty state
            other.initialize_container(); // <-- ИСПРАВЛЕНИЕ 2: Инициализируем other
        } else {
            this->clear();
            copy_container_nodes_from(other);
            other.clear();
        }
    }

    Node<T>* allocate_and_construct_node(const value_type& val, int level) {
        Node<T>* new_node = node_allocator.allocate(1);
        try {
            new (new_node) Node<T>(val, level);
        } catch (...) {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        return new_node;
    }

    Node<T>* allocate_and_construct_sentinel() {
        Node<T>* new_node = node_allocator.allocate(1);
        try {
            new (new_node) Node<T>(true);
        } catch (...) {
            node_allocator.deallocate(new_node, 1);
            throw;
        }
        return new_node;
    }

    void destroy_and_deallocate_node(Node<T>* node) {
        if (node == nullptr) return;
        node->~Node();
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

    void insert_into_skip_list(Node<T>* new_node) {
        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = skip_list_heads[current_max_level];

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < new_node->value) {
                current = current->forward[i];
            }
            update[i] = current;
        }

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
    }

    void remove_from_skip_list(Node<T>* node_to_remove, const value_type& value_for_search) {
        Node<T>* update[MAX_SKIP_LEVEL];
        Node<T>* current = skip_list_heads[current_max_level];

        for (int i = current_max_level; i >= 0; --i) {
            while (current->forward[i] != sentinel_node && current->forward[i]->value < value_for_search) {
                current = current->forward[i];
            }
            update[i] = current;
        }

        current = current->forward[0];

        if (current != sentinel_node && current == node_to_remove) {
            for (int i = 0; i <= current->level; ++i) {
                update[i]->forward[i] = current->forward[i];
            }

            while (current_max_level > 0 && skip_list_heads[current_max_level]->forward[current_max_level] == sentinel_node) {
                current_max_level--;
            }
        }
    }

    Node<T>* find_node_in_skip_list(const value_type& value) const {
        Node<T>* current = skip_list_heads[current_max_level];

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

    // Move Constructor
    Container(Container&& other) noexcept :
        node_allocator(std::move(other.node_allocator)),
        sentinel_node(other.sentinel_node),
        num_elements(other.num_elements),
        skip_list_heads(other.skip_list_heads),
        current_max_level(other.current_max_level),
        rng(std::move(other.rng)),
        dist(std::move(other.dist))
    {
        // Reset other's state to a valid, empty, destructible state
        // Re-initialize 'other' as an empty container
        other.sentinel_node = nullptr; // Temporarily nullify to avoid double deletion during other.initialize_container()
        other.skip_list_heads = nullptr; // Temporarily nullify to avoid double deletion
        other.initialize_container(); // <-- ИСПРАВЛЕНИЕ 3: Инициализируем other
        other.num_elements = 0; // ensure num_elements is zero
    }

    Container(Container&& other, const Allocator& alloc) :
        node_allocator(alloc),
        sentinel_node(nullptr),
        num_elements(0),
        skip_list_heads(nullptr),
        current_max_level(0)
    {
        initialize_container();
        if (node_allocator == other.node_allocator) {
            destroy_container_nodes();

            sentinel_node = other.sentinel_node;
            num_elements = other.num_elements;
            skip_list_heads = other.skip_list_heads;
            current_max_level = other.current_max_level;
            rng = std::move(other.rng);
            dist = std::move(other.dist);

            // Re-initialize 'other' to a valid empty state
            other.sentinel_node = nullptr; // Temporarily nullify
            other.skip_list_heads = nullptr; // Temporarily nullify
            other.initialize_container(); // <-- ИСПРАВЛЕНИЕ 4: Инициализируем other
            other.num_elements = 0;
        } else {
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
            transfer_container_nodes_from(std::move(other));
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
        // Ensure sentinel_node is not null before dereferencing
        if (sentinel_node == nullptr) {
            return iterator(nullptr); // Return a null iterator for safety
        }
        return iterator(sentinel_node->next);
    }

    const_iterator begin() const noexcept {
        // Ensure sentinel_node is not null before dereferencing
        if (sentinel_node == nullptr) {
            return const_iterator(nullptr); // Return a null iterator for safety
        }
        return const_iterator(sentinel_node->next);
    }

    const_iterator cbegin() const noexcept {
        if (sentinel_node == nullptr) {
            return const_iterator(nullptr);
        }
        return const_iterator(sentinel_node->next);
    }

    iterator end() noexcept {
        // Ensure sentinel_node is not null before returning
        if (sentinel_node == nullptr) {
            return iterator(nullptr); // Return a null iterator for safety
        }
        return iterator(sentinel_node);
    }

    const_iterator end() const noexcept {
        if (sentinel_node == nullptr) {
            return const_iterator(nullptr);
        }
        return const_iterator(sentinel_node);
    }

    const_iterator cend() const noexcept {
        if (sentinel_node == nullptr) {
            return const_iterator(nullptr);
        }
        return const_iterator(sentinel_node);
    }

    bool empty() const noexcept {
        // Correctly handle case where sentinel_node might be null (e.g., just after move-from but before re-initialization)
        if (sentinel_node == nullptr) {
            return true; // If no sentinel, it's definitively empty
        }
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

    iterator insert(const_iterator pos, const value_type& value) {
        if (pos.current_node == nullptr) {
            throw std::invalid_argument("Cannot insert at null iterator position.");
        }

        int node_level = get_random_level();
        Node<T>* new_node = allocate_and_construct_node(value, node_level);

        insert_dll_node_before(new_node, const_cast<Node<T>*>(pos.current_node));
        insert_into_skip_list(new_node);

        return iterator(new_node);
    }

    iterator erase(const_iterator pos) {
        if (pos.current_node == nullptr || pos.current_node == sentinel_node) {
            throw std::invalid_argument("Cannot erase at null or sentinel iterator position.");
        }

        Node<T>* node_to_remove = const_cast<Node<T>*>(pos.current_node);
        Node<T>* next_node = node_to_remove->next;

        remove_from_skip_list(node_to_remove, node_to_remove->value);
        remove_dll_node(node_to_remove);

        destroy_and_deallocate_node(node_to_remove);

        return iterator(next_node);
    }

    void push_front(const value_type& value) {
        insert(begin(), value);
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

    void pop_back() {
        if (empty()) {
            throw std::out_of_range("pop_back() called on empty container.");
        }
        erase(--end());
    }

    bool contains(const value_type& value) const {
        return find_node_in_skip_list(value) != nullptr;
    }

    iterator find(const value_type& value) {
        Node<T>* node = const_cast<Node<T>*>(find_node_in_skip_list(value));
        return (node != nullptr) ? iterator(node) : end();
    }

    const_iterator find(const value_type& value) const {
        Node<T>* node = find_node_in_skip_list(value);
        return (node != nullptr) ? const_iterator(node) : end();
    }
};

#endif // CONTAINER_CONTAINER_H