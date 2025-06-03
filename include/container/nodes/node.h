#ifndef CONTAINER_NODES_NODE_H
#define CONTAINER_NODES_NODE_H

#include <vector> // For skip list forward pointers
#include <new>    // For placement new

// Forward declaration of Container to access MAX_SKIP_LEVEL, if needed
// However, the Node structure itself doesn't need MAX_SKIP_LEVEL directly for its construction anymore.
// It's mainly used by Container's helpers.

template <typename T>
struct Node {
    T value;
    Node* prev;
    Node* next;
    std::vector<Node*> forward; // Pointers to next nodes at different levels
    int level; // Current level of this node in the skip list
    bool is_sentinel; // Indicates if this is the special sentinel node

    // Constructor for sentinel node
    explicit Node(bool sentinel = false);

    // Constructor for regular data node
    explicit Node(const T& val, int node_level);

    // Destructor to explicitly destroy the value if it's not trivial
    ~Node();

    // Disable copy/move for Node; Container manages node lifecycle
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;
};

#endif // CONTAINER_NODES_NODE_H