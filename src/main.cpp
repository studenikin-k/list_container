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

    // --- Copy & Move Semantics Test ---
    std::cout << "\n--- Copy & Move Semantics Test ---" << std::endl;
    Container<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);
    print_container_state("Original", original);

    Container<int> copied = original; // Copy Constructor
    print_container_state("Copied (from Original)", copied);
    original.clear();
    print_container_state("Original (after clear)", original);
    print_container_state("Copied (should be unchanged)", copied);

    Container<int> moved = std::move(copied); // Move Constructor
    print_container_state("Moved (from Copied)", moved);
    print_container_state("Copied (after move, empty)", copied); // Should be empty

    Container<int> assigned_copy;
    assigned_copy.push_back(99); // Some initial data
    assigned_copy = moved; // Copy Assignment
    print_container_state("Assigned_Copy (from Moved)", assigned_copy);

    Container<int> assigned_move;
    assigned_move.push_back(88); // Some initial data
    assigned_move = std::move(moved); // Move Assignment
    print_container_state("Assigned_Move (from Moved)", assigned_move);
    print_container_state("Moved (after move assign, empty)", moved); // Should be empty

    // --- Constructor Test ---
    std::cout << "\n--- Constructor Test ---" << std::endl;
    Container<std::string> s_list = {"apple", "banana", "cherry"};
    print_container_state("String list (initializer_list)", s_list); // Теперь работает!

    Container<double> d_list(3); // 3 default-constructed doubles (0.0)
    print_container_state("Double list (count)", d_list); // Теперь работает!

    Container<char> char_list(5, 'X'); // 5 'X' characters
    print_container_state("Char list (count, value)", char_list); // Теперь работает!


    std::cout << "\nAll checks completed." << std::endl;
    return 0;
}