#include "container/container.h" // Main header
#include "container/nodes/node.h" // Node definition

// Include headers for helper implementations (these will be .cpp files too)
#include "helpers/helpers.cpp" // Includes initialize_container, destroy_container_nodes, alloc/dealloc, etc.
#include "dll_ops/dll_ops.cpp" // Includes DLL operations
#include "skip_ops/skip_ops.cpp" // Includes Skip List operations

// --- Iterator Definitions (must be in .cpp if not inline) ---
template <typename T, typename Allocator>
template <bool IsConst>
Container<T, Allocator>::Iterator<IsConst>::Iterator() : current_node(nullptr) {}

template <typename T, typename Allocator>
template <bool IsConst>
Container<T, Allocator>::Iterator<IsConst>::operator Iterator<true>() const noexcept requires (!IsConst) {
    return Iterator<true>(current_node);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Container<T, Allocator>::template Iterator<IsConst>::reference Container<T, Allocator>::Iterator<IsConst>::operator*() const {
    if (!current_node || current_node->is_sentinel) {
        throw std::out_of_range("Dereferencing invalid iterator or sentinel.");
    }
    return current_node->value;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Container<T, Allocator>::template Iterator<IsConst>::pointer Container<T, Allocator>::Iterator<IsConst>::operator->() const {
    if (!current_node || current_node->is_sentinel) {
        throw std::out_of_range("Dereferencing invalid iterator or sentinel.");
    }
    return std::addressof(current_node->value);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Container<T, Allocator>::template Iterator<IsConst>& Container<T, Allocator>::Iterator<IsConst>::operator++() { // Pre-increment
    if (!current_node) {
        throw std::out_of_range("Incrementing null iterator.");
    }
    current_node = current_node->next;
    return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Container<T, Allocator>::template Iterator<IsConst> Container<T, Allocator>::Iterator<IsConst>::operator++(int) { // Post-increment
    Iterator temp = *this;
    ++(*this);
    return temp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Container<T, Allocator>::template Iterator<IsConst>& Container<T, Allocator>::Iterator<IsConst>::operator--() { // Pre-decrement
    if (!current_node) {
        throw std::out_of_range("Decrementing null iterator.");
    }
    current_node = current_node->prev;
    return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Container<T, Allocator>::template Iterator<IsConst> Container<T, Allocator>::Iterator<IsConst>::operator--(int) { // Post-decrement
    Iterator temp = *this;
    --(*this);
    return temp;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Container<T, Allocator>::Iterator<IsConst>::operator==(const Iterator& other) const noexcept {
    return current_node == other.current_node;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Container<T, Allocator>::Iterator<IsConst>::operator!=(const Iterator& other) const noexcept {
    return !(*this == other);
}


// --- Constructors and Destructor Implementation ---

template <typename T, typename Allocator>
Container<T, Allocator>::Container(const Allocator& alloc) :
    node_allocator(alloc),
    sentinel_node(nullptr),
    num_elements(0),
    skip_list_heads(nullptr),
    current_max_level(0)
{
    initialize_container();
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(size_type count, const value_type& value, const Allocator& alloc) :
    Container(alloc) // Delegate to default constructor
{
    for (size_type i = 0; i < count; ++i) {
        push_back(value);
    }
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(size_type count, const Allocator& alloc) :
    Container(alloc) // Delegate to default constructor
{
    for (size_type i = 0; i < count; ++i) {
        push_back(T()); // Default construct elements
    }
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(std::initializer_list<value_type> init, const Allocator& alloc) :
    Container(alloc) // Delegate to default constructor
{
    for (const auto& val : init) {
        push_back(val);
    }
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(const Container& other) :
    node_allocator(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator())),
    sentinel_node(nullptr),
    num_elements(0),
    skip_list_heads(nullptr),
    current_max_level(0)
{
    initialize_container(); // Initialize our own sentinel
    copy_container_nodes_from(other);
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(const Container& other, const Allocator& alloc) :
    node_allocator(alloc),
    sentinel_node(nullptr),
    num_elements(0),
    skip_list_heads(nullptr),
    current_max_level(0)
{
    initialize_container();
    copy_container_nodes_from(other);
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(Container&& other) noexcept :
    node_allocator(std::move(other.node_allocator)),
    sentinel_node(other.sentinel_node),
    num_elements(other.num_elements),
    skip_list_heads(other.skip_list_heads),
    current_max_level(other.current_max_level)
{
    // Reset other's state
    other.sentinel_node = nullptr;
    other.num_elements = 0;
    other.skip_list_heads = nullptr;
    other.current_max_level = 0;
}

template <typename T, typename Allocator>
Container<T, Allocator>::Container(Container&& other, const Allocator& alloc) :
    node_allocator(alloc),
    sentinel_node(nullptr),
    num_elements(0),
    skip_list_heads(nullptr),
    current_max_level(0)
{
    initialize_container(); // Always initialize our own
    if (node_allocator == other.node_allocator) {
        // Same allocator, can steal resources
        destroy_container_nodes(); // Clean up initial state
        sentinel_node = other.sentinel_node;
        num_elements = other.num_elements;
        skip_list_heads = other.skip_list_heads;
        current_max_level = other.current_max_level;

        other.sentinel_node = nullptr;
        other.num_elements = 0;
        other.skip_list_heads = nullptr;
        other.current_max_level = 0;
    } else {
        // Different allocators, must copy elements
        copy_container_nodes_from(other);
        other.clear();
    }
}

template <typename T, typename Allocator>
Container<T, Allocator>::~Container() {
    destroy_container_nodes();
}

// --- Assignment Operators ---

template <typename T, typename Allocator>
Container<T, Allocator>& Container<T, Allocator>::operator=(const Container& other) {
    if (this != &other) {
        // Handle allocator propagation on copy assignment
        if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value &&
            node_allocator != other.node_allocator) {
            // New allocator adopted, deallocate current using old allocator first
            destroy_container_nodes();
            node_allocator = other.node_allocator;
            initialize_container(); // Re-initialize with new allocator
        } else {
            clear(); // Clear current content but keep allocator
        }
        copy_container_nodes_from(other);
    }
    return *this;
}

template <typename T, typename Allocator>
Container<T, Allocator>& Container<T, Allocator>::operator=(Container&& other) noexcept {
    if (this != &other) {
        destroy_container_nodes(); // Clean up current resources
        transfer_container_nodes_from(std::move(other)); // Transfer or copy based on allocator
    }
    return *this;
}

// --- Allocator ---
template <typename T, typename Allocator>
typename Container<T, Allocator>::allocator_type Container<T, Allocator>::get_allocator() const noexcept {
    return node_allocator;
}

// --- Element Access ---

template <typename T, typename Allocator>
typename Container<T, Allocator>::reference Container<T, Allocator>::front() {
    if (empty()) {
        throw std::out_of_range("front() called on empty container.");
    }
    return sentinel_node->next->value;
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_reference Container<T, Allocator>::front() const {
    if (empty()) {
        throw std::out_of_range("front() called on empty container.");
    }
    return sentinel_node->next->value;
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::reference Container<T, Allocator>::back() {
    if (empty()) {
        throw std::out_of_range("back() called on empty container.");
    }
    return sentinel_node->prev->value;
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_reference Container<T, Allocator>::back() const {
    if (empty()) {
        throw std::out_of_range("back() called on empty container.");
    }
    return sentinel_node->prev->value;
}

// --- Iterators ---

template <typename T, typename Allocator>
typename Container<T, Allocator>::iterator Container<T, Allocator>::begin() noexcept {
    return iterator(sentinel_node->next);
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_iterator Container<T, Allocator>::begin() const noexcept {
    return const_iterator(sentinel_node->next);
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_iterator Container<T, Allocator>::cbegin() const noexcept {
    return const_iterator(sentinel_node->next);
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::iterator Container<T, Allocator>::end() noexcept {
    return iterator(sentinel_node);
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_iterator Container<T, Allocator>::end() const noexcept {
    return const_iterator(sentinel_node);
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_iterator Container<T, Allocator>::cend() const noexcept {
    return const_iterator(sentinel_node);
}

// --- Capacity ---

template <typename T, typename Allocator>
bool Container<T, Allocator>::empty() const noexcept {
    return num_elements == 0;
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::size_type Container<T, Allocator>::size() const noexcept {
    return num_elements;
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::size_type Container<T, Allocator>::max_size() const noexcept {
    return std::allocator_traits<NodeAllocator>::max_size(node_allocator);
}

// --- Modifiers ---

template <typename T, typename Allocator>
void Container<T, Allocator>::clear() noexcept {
    destroy_container_nodes();
    initialize_container();
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::iterator
Container<T, Allocator>::insert(const_iterator pos, const value_type& value) {
    if (pos.current_node == nullptr) {
        throw std::invalid_argument("Cannot insert at null iterator position.");
    }

    int node_level = get_random_level();
    Node<T>* new_node = allocate_and_construct_node(value, node_level);

    // DLL part
    insert_dll_node_before(new_node, pos.current_node);

    // Skip List part
    insert_into_skip_list(new_node);

    return iterator(new_node);
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::iterator
Container<T, Allocator>::erase(const_iterator pos) {
    if (pos.current_node == nullptr || pos.current_node == sentinel_node) {
        throw std::invalid_argument("Cannot erase at null or sentinel iterator position.");
    }

    Node<T>* node_to_remove = pos.current_node;
    Node<T>* next_node = node_to_remove->next; // Save for return value

    // Skip List part (needs value for path finding before removing from DLL)
    remove_from_skip_list(node_to_remove, node_to_remove->value);

    // DLL part
    remove_dll_node(node_to_remove);

    // Deallocate node
    destroy_and_deallocate_node(node_to_remove);

    return iterator(next_node);
}

template <typename T, typename Allocator>
void Container<T, Allocator>::push_front(const value_type& value) {
    insert(begin(), value);
}

template <typename T, typename Allocator>
void Container<T, Allocator>::pop_front() {
    if (empty()) {
        throw std::out_of_range("pop_front() called on empty container.");
    }
    erase(begin());
}

template <typename T, typename Allocator>
void Container<T, Allocator>::push_back(const value_type& value) {
    insert(end(), value);
}

template <typename T, typename Allocator>
void Container<T, Allocator>::pop_back() {
    if (empty()) {
        throw std::out_of_range("pop_back() called on empty container.");
    }
    erase(--end()); // Erase the node before the sentinel
}

// --- Skip List specific modifiers/accessors ---

template <typename T, typename Allocator>
bool Container<T, Allocator>::contains(const value_type& value) const {
    return find_node_in_skip_list(value) != nullptr;
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::iterator Container<T, Allocator>::find(const value_type& value) {
    Node<T>* node = find_node_in_skip_list(value);
    return (node != nullptr) ? iterator(node) : end();
}

template <typename T, typename Allocator>
typename Container<T, Allocator>::const_iterator Container<T, Allocator>::find(const value_type& value) const {
    Node<T>* node = find_node_in_skip_list(value);
    return (node != nullptr) ? const_iterator(node) : end();
}

