#ifndef CONTAINER_NODES_NODE_H
#define CONTAINER_NODES_NODE_H

#include <memory>
#include <utility>

template <typename T>
struct Node {
    T value;              // 1. Declare 'value' first
    Node<T>* next;        // 2. Declare 'next' second
    Node<T>* prev;        // 3. Declare 'prev' third
    Node<T>** forward;    // 4. Declare 'forward' fourth
    int level;            // 5. Declare 'level' fifth
    bool is_sentinel;     // 6. Declare 'is_sentinel' last (or at least after members it depends on)

    // Constructor for regular nodes (value by const reference)
    explicit Node(const T& val, int node_level)
        : value(val),         // Initialize in declared order
          next(nullptr),      // Initialize in declared order
          prev(nullptr),      // Initialize in declared order
          forward(new Node<T>*[node_level + 1]), // Initialize in declared order
          level(node_level),  // Initialize in declared order
          is_sentinel(false)  // Initialize in declared order
    {
        // Проверка node_level >= 0 для безопасного выделения
        if (node_level < 0) {
            // В идеале, бросить исключение или обработать как ошибку
            // Но для Skip List уровень всегда должен быть >= 0
            // Здесь предполагаем, что `get_random_level()` обеспечивает валидный `node_level`.
        }
        for (int i = 0; i <= node_level; ++i) {
            forward[i] = nullptr;
        }
    }

    // Constructor for regular nodes (value by rvalue reference - for moving)
    explicit Node(T&& val, int node_level)
        : value(std::move(val)), // Initialize in declared order
          next(nullptr),         // Initialize in declared order
          prev(nullptr),         // Initialize in declared order
          forward(new Node<T>*[node_level + 1]), // Initialize in declared order
          level(node_level),     // Initialize in declared order
          is_sentinel(false)     // Initialize in declared order
    {
        if (node_level < 0) {
            // См. комментарий выше
        }
        for (int i = 0; i <= node_level; ++i) {
            forward[i] = nullptr;
        }
    }

    // Constructor for sentinel node
    explicit Node(bool sentinel_flag, int max_forward_levels_size)
        : value(), // Default-construct T (for int this is 0, for string this is "")
          next(nullptr),
          prev(nullptr),
          forward(nullptr), // Initialize forward here, then conditionally allocate below
          level(max_forward_levels_size - 1), // Sentinel has level MAX_SKIP_LEVEL - 1 (or 0 if MAX_SKIP_LEVEL=1)
          is_sentinel(sentinel_flag)
    {
        if (sentinel_flag) {
            // Allocate memory for the forward array for MAX_SKIP_LEVEL levels
            // if this is a sentinel node.
            // Note: `forward` was initialized to nullptr in the initializer list
            // to conform to the order, now we reassign it if it's a sentinel.
            // Проверка max_forward_levels_size для безопасного выделения
            if (max_forward_levels_size <= 0) {
                // Это критическая ошибка, MAX_SKIP_LEVEL должен быть > 0
                // Можно бросить исключение или вызвать terminate
                forward = nullptr; // Убедимся, что forward по-прежнему nullptr, если size невалиден
                // или бросить std::bad_alloc / std::length_error
            } else {
                forward = new Node<T>*[max_forward_levels_size];
                for (int i = 0; i < max_forward_levels_size; ++i) {
                    forward[i] = nullptr; // Initialize
                }
            }
        }
    }

    // Destructor
    ~Node() {
        delete[] forward; // Free memory allocated for the forward array
        // The value (T value) is destructed automatically
    }

    // Default constructor is deleted, as there are custom ones.
    Node() = delete; // Disallow default constructor
};

#endif // CONTAINER_NODES_NODE_H