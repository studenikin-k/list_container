#include "container/container.h"
#include "container/nodes/node.h"



template <typename T, typename Allocator>
void Container<T, Allocator>::insert_dll_node_before(Node<T>* new_node, Node<T>* position_node) {
    Node<T>* prev_node = position_node->prev;

    new_node->next = position_node;
    new_node->prev = prev_node;
    prev_node->next = new_node;
    position_node->prev = new_node;

    num_elements++;
}

template <typename T, typename Allocator>
void Container<T, Allocator>::remove_dll_node(Node<T>* node_to_remove) {
    node_to_remove->prev->next = node_to_remove->next;
    node_to_remove->next->prev = node_to_remove->prev;
    num_elements--;
}