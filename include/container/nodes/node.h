#ifndef CONTAINER_NODES_NODE_H
#define CONTAINER_NODES_NODE_H

#include <vector> // For skip list forward pointers
#include <new>    // For placement new (needed if T is not trivially constructible/destructible)

// Forward declaration of Container is generally not needed here if Node's implementation
// doesn't directly depend on Container's internal members like MAX_SKIP_LEVEL for its own logic.
// Container uses Node, but Node doesn't need to know about Container's internals.

template <typename T>
struct Node {
    // T value; // Moved into a union to manage lifetime with placement new
    union {
        T value;
    };

    Node* prev;
    Node* next;
    std::vector<Node*> forward; // Pointers to next nodes at different levels
    int level; // Current level of this node in the skip list
    bool is_sentinel; // Indicates if this is the special sentinel node

    // Constructor for sentinel node
    explicit Node(bool sentinel = false) :
        prev(nullptr),
        next(nullptr),
        forward(0), // Initial size, will be resized by Container::initialize_container for sentinel
        level(-1),  // Sentinel nodes usually have a level of -1 or MAX_SKIP_LEVEL
        is_sentinel(sentinel)
    {
        if (is_sentinel) {
            // Sentinel's value is typically not used, but if T requires construction,
            // or if you want to be pedantic about valid state for all members,
            // you might default-construct it. However, the value of a sentinel
            // is usually ignored.
            // new (&value) T(); // Only if T needs construction even for unused sentinel value
            prev = this; // Self-referencing for an empty list
            next = this; // Self-referencing for an empty list
        }
    }

    // Constructor for regular data node
    explicit Node(const T& val, int node_level) :
        prev(nullptr),
        next(nullptr),
        forward(node_level + 1, nullptr), // Initialize forward pointers with size for given level
        level(node_level),
        is_sentinel(false)
    {
        // Use placement new to construct 'value' in the pre-allocated raw memory
        new (&value) T(val);
    }

    // Destructor to explicitly destroy the value if T is not trivially destructible
    ~Node() {
        if (!is_sentinel) { // Don't call destructor for sentinel's "value"
            value.~T(); // Explicitly call destructor for the contained value
        }
    }

    // Disable copy/move for Node; Container manages node lifecycle
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;
};

#endif // CONTAINER_NODES_NODE_H