#ifndef CONTAINER_CONTAINER_H
#define CONTAINER_CONTAINER_H

#include <memory>       // For std::allocator, std::addressof
#include <cstddef>      // For std::ptrdiff_t, std::size_t
#include <iterator>     // For std::bidirectional_iterator_tag
#include <stdexcept>    // For exceptions
#include <initializer_list> // For std::initializer_list
#include <type_traits>  // For std::conditional_t, std::enable_if_t
#include "container/nodes/node.h" // <-- Важно: теперь мы включаем полное определение Node

// Удалена forward declaration struct Node; так как теперь мы включаем полный заголовочный файл

template <typename T, typename Allocator = std::allocator<T>>
class Container {
public:
    // --- Typedefs required by STL Container concepts ---
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    // --- Iterator definitions ---
    template <bool IsConst>
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = Container::value_type;
        using difference_type = Container::difference_type;
        // Pointer and reference types depend on IsConst
        using pointer = std::conditional_t<IsConst, Container::const_pointer, Container::pointer>;
        using reference = std::conditional_t<IsConst, Container::const_reference, Container::reference>;

        // Internal node type - now correctly conditional
        using NodeType = Node<value_type>;
        using NodePointer = std::conditional_t<IsConst, const NodeType*, NodeType*>;

    private:
        NodePointer current_node;

        // Allow Container to create iterators
        friend class Container;

        // Private constructor for internal use by Container
        explicit Iterator(NodePointer node) : current_node(node) {}

    public:
        // Default constructor - now only a declaration, definition will be in .cpp
        Iterator(); // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)

        // Templated constructor for iterator conversion - only a declaration
        template<bool OtherIsConst, typename = std::enable_if_t<IsConst || !OtherIsConst>>
        Iterator(const Iterator<OtherIsConst>& other) noexcept; // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)

        // Conversion operator - only a declaration
        operator Iterator<true>() const noexcept requires (!IsConst); // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)

        reference operator*() const; // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)
        pointer operator->() const; // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)

        Iterator& operator++(); // Pre-increment <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)
        Iterator operator++(int); // Post-increment <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)
        Iterator& operator--(); // Pre-decrement <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)
        Iterator operator--(int); // Post-decrement <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)

        bool operator==(const Iterator& other) const noexcept; // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)
        bool operator!=(const Iterator& other) const noexcept; // <--- ОБЪЯВЛЕНИЕ (definition moved to .cpp)
    };

    // Public iterator types
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

private:
    // Allocator for Node<T>
    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node<T>>;
    NodeAllocator node_allocator;

    Node<T>* sentinel_node; // Sentinel node for circular doubly linked list
    size_type num_elements;

    // --- Skip List Specifics ---
    static constexpr int MAX_SKIP_LEVEL = 16;
    Node<T>** skip_list_heads; // Array of pointers to nodes at different levels
    int current_max_level;

    // --- Private Helper Method Declarations (Implementations in separate .cpp files) ---

    // General Helpers (in helpers.cpp)
    void initialize_container();
    void destroy_container_nodes();
    void copy_container_nodes_from(const Container& other);
    void transfer_container_nodes_from(Container&& other) noexcept;

    // Node Allocation/Deallocation (in helpers.cpp)
    Node<T>* allocate_and_construct_node(const value_type& val, int level);
    Node<T>* allocate_and_construct_sentinel();
    void destroy_and_deallocate_node(Node<T>* node);

    // Doubly Linked List Specific (in dll_ops.cpp)
    void insert_dll_node_before(Node<T>* new_node, Node<T>* position_node);
    void remove_dll_node(Node<T>* node_to_remove);

    // Skip List Specific (in skip_ops.cpp)
    int get_random_level() const;
    void insert_into_skip_list(Node<T>* new_node);
    void remove_from_skip_list(Node<T>* node_to_remove, const value_type& value_for_search);
    Node<T>* find_node_in_skip_list(const value_type& value) const;

public:
    // --- Constructors and Destructor ---
    explicit Container(const Allocator& alloc = Allocator());
    Container(size_type count, const value_type& value, const Allocator& alloc = Allocator());
    explicit Container(size_type count, const Allocator& alloc = Allocator());
    Container(std::initializer_list<value_type> init, const Allocator& alloc = Allocator());

    // Copy Constructor
    Container(const Container& other);
    Container(const Container& other, const Allocator& alloc);

    // Move Constructor
    Container(Container&& other) noexcept;
    Container(Container&& other, const Allocator& alloc);

    // Destructor
    ~Container();

    // --- Assignment Operators ---
    Container& operator=(const Container& other);
    Container& operator=(Container&& other) noexcept;

    // --- Allocator ---
    allocator_type get_allocator() const noexcept;

    // --- Element Access ---
    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;

    // --- Iterators ---
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    // --- Capacity ---
    bool empty() const noexcept;
    size_type size() const noexcept;
    size_type max_size() const noexcept;

    // --- Modifiers ---
    void clear() noexcept;
    iterator insert(const_iterator pos, const value_type& value);
    iterator erase(const_iterator pos);
    void push_front(const value_type& value);
    void pop_front();
    void push_back(const value_type& value);
    void pop_back();

    // --- Skip List specific modifiers/accessors ---
    bool contains(const value_type& value) const;
    iterator find(const value_type& value);
    const_iterator find(const value_type& value) const;
};

#endif // CONTAINER_CONTAINER_H