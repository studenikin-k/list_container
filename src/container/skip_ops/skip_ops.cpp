#include <iostream>
#include <string>
#include <vector> // Возможно, не нужна, если не используется явно в этом фрагменте
#include <numeric> // Возможно, не нужна, если не используется явно в этом фрагменте
#include "container/container.h" // Убедитесь, что этот заголовок включен, если он содержит определение Container

template <typename T, typename Allocator = std::allocator<T>> // <--- ДОБАВЛЕНИЕ ЭТОЙ СТРОКИ
void print_container(const std::string& name, const Container<T, Allocator>& cont) {
    std::cout << name << ": { "; // Добавлен пробел для лучшей читаемости
    bool first = true;
    for (const auto& val : cont) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << val;
        first = false;
    }
    std::cout << "} Size: " << cont.size() << std::endl;
}