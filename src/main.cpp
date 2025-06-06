#include <iostream>
#include <string>
#include <vector>     // Хотя и не используется активно, оставим для возможных вспомогательных нужд
#include <numeric>    // Для std::iota, если нужно
#include <iterator>   // Для std::advance
#include "../include/container/container.h" // Подключаем ваш контейнер

// Вспомогательная функция для печати контейнера (ШАБЛОННАЯ)
template <typename T>
void print_container_state(const std::string& name, const Container<T>& cont) {
    std::cout << name << " { ";
    bool first = true;
    for (const auto& val : cont) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << val;
        first = false;
    }
    std::cout << " } Size: " << cont.size();
    std::cout << (cont.empty() ? " (Empty)" : "") << std::endl;
}

int main() {
    // --- Basic Operations Test ---
    std::cout << "\n--- Basic Operations Test ---" << std::endl;
    Container<int> c1;
    print_container_state("Initial", c1);

    c1.push_back(10);
    c1.push_front(5);
    c1.push_back(20);
    c1.push_front(0);
    print_container_state("After push", c1);
    std::cout << "Front: " << c1.front() << ", Back: " << c1.back() << std::endl;

    c1.pop_front();
    print_container_state("After pop_front", c1);
    c1.pop_back();
    print_container_state("After pop_back", c1);
    c1.clear();
    print_container_state("After clear", c1);

    // --- Iterator & Middle Ops Test ---
    std::cout << "\n--- Iterator & Middle Ops Test ---" << std::endl;
    Container<int> c2;
    for (int i = 0; i < 5; ++i) {
        c2.push_back(i * 10); // 0, 10, 20, 30, 40
    }
    print_container_state("Initial", c2);

    std::cout << "Forward iter: ";
    for (auto it = c2.begin(); it != c2.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    std::cout << "Backward iter: ";
    for (auto it = c2.end(); it != c2.begin(); ) {
        --it;
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    auto it_middle = c2.begin();
    std::advance(it_middle, 2); // -> 20
    c2.insert(it_middle, 15);
    print_container_state("After insert 15 at pos 2", c2);

    auto it_to_erase = c2.find(20); // Find 20
    if (it_to_erase != c2.end()) {
        c2.erase(it_to_erase);
    }
    print_container_state("After erase 20", c2);

    // --- Skip List Features Test ---
    std::cout << "\n--- Skip List Features Test ---" << std::endl;
    Container<int> c3;
    for (int i = 0; i < 10; ++i) {
        c3.push_back(i * 5); // 0, 5, 10, ..., 45
    }
    print_container_state("Test List", c3);

    std::cout << "Contains 25? " << (c3.contains(25) ? "Yes" : "No") << std::endl;
    std::cout << "Contains 100? " << (c3.contains(100) ? "Yes" : "No") << std::endl;

    auto found = c3.find(15);
    if (found != c3.end()) {
        std::cout << "Found 15. Value: " << *found << std::endl;
    } else {
        std::cout << "15 not found." << std::endl;
    }
    found = c3.find(50);
    if (found != c3.end()) {
        std::cout << "Found 50. Value: " << *found << std::endl;
    } else {
        std::cout << "50 not found." << std::endl;
    }


    return 0;
}