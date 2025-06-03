#include "container/container.h"
#include "container/nodes/node.h" // For Node definition
#include <algorithm> // For std::swap

// Implementation for Node constructors/destructor (kept here for consistency, though can be in node.h)
template <typename T>
Node<T>::Node(bool sentinel) :
    prev(nullptr),
    next(nullptr),
    forward(0),
    level(-1),
    is_sentinel(sentinel)
{
    if (is_sentinel) {
        new (&value) T(); // Placement new for T (default construct, as it's not used)
        prev = this;
        next = this;
        // forward vector for sentinel will be sized and populated in initialize_container
    }
}

template <typename T>
Node<T>::Node(const T& val, int node_level) :
    prev(nullptr),
    next(nullptr),
    forward(node_level + 1, nullptr), // Initialize forward pointers
    level(node_level),
    is_sentinel(false)
{
    new (&value) T(val); // Placement new to construct the value
}

template <typename T>
Node<T>::~Node() {
    if (!is_sentinel) {
        value.~T(); // Explicitly call destructor for value
    }
}

// --- Private Helper Methods Implementation (General) ---

template <typename T, typename Allocator>
void Container<T, Allocator>::initialize_container() {
    sentinel_node = allocate_and_construct_sentinel();
    num_elements = 0;

    // Initialize skip list heads to point to the sentinel node at all levels
    sentinel_node->forward.assign(MAX_SKIP_LEVEL, sentinel_node); // Ensure sentinel's forward is sized
    skip_list_heads = new Node<T>*[MAX_SKIP_LEVEL];
    for (int i = 0; i < MAX_SKIP_LEVEL; ++i) {
        skip_list_heads[i] = sentinel_node;
    }
    current_max_level = 0; // No actual data nodes yet
}

template <typename T, typename Allocator>
void Container<T, Allocator>::destroy_container_nodes() {
    if (sentinel_node == nullptr) return;

    Node<T>* current = sentinel_node->next;
    while (current != sentinel_node) {
        Node<T>* next_node = current->next;
        destroy_and_deallocate_node(current);
        current = next_node;
    }
    // Destroy and deallocate the sentinel node itself
    destroy_and_deallocate_node(sentinel_node);
    sentinel_node = nullptr;

    // Deallocate skip list head array
    delete[] skip_list_heads;
    skip_list_heads = nullptr;
}

template <typename T, typename Allocator>
void Container<T, Allocator>::copy_container_nodes_from(const Container& other) {
    for (const auto& val : other) {
        push_back(val); // This will handle DLL and Skip List insertion
    }
}

template <typename T, typename Allocator>
void Container<T, Allocator>::transfer_container_nodes_from(Container&& other) noexcept {
    if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
        node_allocator == other.node_allocator) {
        // Simple swap if allocators are compatible or propagation is allowed
        std::swap(sentinel_node, other.sentinel_node);
        std::swap(num_elements, other.num_elements);
        std::swap(skip_list_heads, other.skip_list_heads);
        std::swap(current_max_level, other.current_max_level);

        // Reset other's state to a valid empty container
        other.initialize_container();
        other.num_elements = 0;
        other.current_max_level = 0;
    } else {
        // Different allocators, must perform deep copy
        clear(); // Clear our current content
        copy_container_nodes_from(other); // Copy elements
        other.clear(); // Clear the source container
    }
}

// --- Private Helper Methods (Node Allocation/Deallocation) ---

template <typename T, typename Allocator>
Node<T>* Container<T, Allocator>::allocate_and_construct_node(const value_type& val, int level) {
    Node<T>* new_node = node_allocator.allocate(1);
    try {
        new (new_node) Node<T>(val, level); // Placement new to construct Node
    } catch (...) {
        node_allocator.deallocate(new_node, 1);
        throw;
    }
    return new_node;
}

template <typename T, typename Allocator>
Node<T>* Container<T, Allocator>::allocate_and_construct_sentinel() {
    Node<T>* new_node = node_allocator.allocate(1);
    try {
        new (new_node) Node<T>(true); // Placement new to construct Node sentinel
    } catch (...) {
        node_allocator.deallocate(new_node, 1);
        throw;
    }
    return new_node;
}

template <typename T, typename Allocator>
void Container<T, Allocator>::destroy_and_deallocate_node(Node<T>* node) {
    if (node == nullptr) return;
    node->~Node(); // Explicitly call destructor for Node
    node_allocator.deallocate(node, 1);
}