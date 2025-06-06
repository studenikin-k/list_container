// container/nodes/node.h
#ifndef CONTAINER_NODES_NODE_H
#define CONTAINER_NODES_NODE_H

#include <cstddef> // Для std::size_t
#include <utility> // Для std::move

template <typename T>
struct Node {
    T value;
    Node<T>* next; // For DLL part
    Node<T>* prev; // For DLL part
    Node<T>** forward; // For Skip List part
    int level;
    bool is_sentinel; // True if it's the sentinel node

    // Убедитесь, что MAX_SKIP_LEVEL определен где-то, например, в Container
    // или передан как параметр шаблона в Node, если Node не инстанцируется в Container.
    // Для простоты, если MAX_SKIP_LEVEL константа, ее можно захардкодить здесь,
    // но лучше использовать значение из Container.
    // Допустим, она будет 16, как в Container.
    static constexpr int MAX_NODE_LEVEL = 16; // Должен совпадать с Container::MAX_SKIP_LEVEL

    // Constructor for regular nodes
    Node(const T& val, int node_level) :
        value(val), next(nullptr), prev(nullptr), level(node_level), is_sentinel(false) {
        forward = new Node<T>*[MAX_NODE_LEVEL]; // Выделяем память для всех уровней
        for (int i = 0; i < MAX_NODE_LEVEL; ++i) { // Инициализируем все nullptr
            forward[i] = nullptr;
        }
    }

    // Constructor for regular nodes (move)
    Node(T&& val, int node_level) :
        value(std::move(val)), next(nullptr), prev(nullptr), level(node_level), is_sentinel(false) {
        forward = new Node<T>*[MAX_NODE_LEVEL];
        for (int i = 0; i < MAX_NODE_LEVEL; ++i) {
            forward[i] = nullptr;
        }
    }

    // Constructor for sentinel node
    Node(bool sentinel = true, int node_level = MAX_NODE_LEVEL) : // Sentinel всегда имеет MAX_NODE_LEVEL
        value(T()), next(nullptr), prev(nullptr), level(node_level), is_sentinel(sentinel) {
        forward = new Node<T>*[MAX_NODE_LEVEL];
        for (int i = 0; i < MAX_NODE_LEVEL; ++i) { // Инициализируем все nullptr
            forward[i] = nullptr;
        }
    }

    ~Node() {
        delete[] forward;
    }

    // Удаляем конструктор копирования и оператор присваивания копированием,
    // чтобы избежать двойного удаления или некорректного копирования.
    // Nodes должны управляться аллокатором контейнера.
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;
};

#endif // CONTAINER_NODES_NODE_H