#include <iostream>
#include <string>
#include <vector>
#include <numeric> // For std::iota
#include "container/container.h" // Include your container header

// Helper function to print the container
template <typename T>
void print_container(const std::string& name, const Container<T>& cont) {
    std::cout << name << ": {";
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

int main() {
    std::cout << "--- Container Basic Operations ---" << std::endl;

    Container<int> list1;
    print_container("list1 (empty)", list1);
    std::cout << "list1 empty? " << (list1.empty() ? "Yes" : "No") << std::endl;

    list1.push_back(10);
    list1.push_front(5);
    list1.push_back(20);
    list1.push_front(0);
    print_container("list1 after push", list1);

    std::cout << "Front: " << list1.front() << ", Back: " << list1.back() << std::endl;

    list1.pop_front();
    print_container("list1 after pop_front", list1);
    list1.pop_back();
    print_container("list1 after pop_back", list1);

    // --- Iterators ---
    std::cout << "\n--- Iterator Test ---" << std::endl;
    Container<int> list2;
    for (int i = 0; i < 5; ++i) {
        list2.push_back(i * 10);
    }
    print_container("list2", list2);

    std::cout << "Iterating forwards: ";
    for (auto it = list2.begin(); it != list2.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    std::cout << "Iterating backwards: ";
    for (auto it = list2.end(); it != list2.begin(); ) {
        --it;
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Insert at specific position
    auto it_insert = list2.begin();
    std::advance(it_insert, 2); // Move 2 steps forward (to element 20)
    list2.insert(it_insert, 15);
    print_container("list2 after insert 15 at pos 2", list2);

    // Erase at specific position
    auto it_erase = list2.find(20); // Find 20 using skip list find
    if (it_erase != list2.end()) {
        list2.erase(it_erase);
    }
    print_container("list2 after erase 20", list2);


    // --- Copy and Move Semantics ---
    std::cout << "\n--- Copy and Move Semantics ---" << std::endl;
    Container<int> list3 = list2; // Copy constructor
    print_container("list3 (copy of list2)", list3);
    list2.clear();
    print_container("list2 after clear", list2);
    print_container("list3 (should be unchanged)", list3);

    Container<int> list4 = std::move(list3); // Move constructor
    print_container("list4 (move of list3)", list4);
    print_container("list3 (after move, should be empty)", list3); // list3 is now empty

    Container<int> list5;
    list5 = list4; // Copy assignment
    print_container("list5 (copy assign from list4)", list5);

    Container<int> list6;
    list6 = std::move(list4); // Move assignment
    print_container("list6 (move assign from list4)", list6);
    print_container("list4 (after move assign, should be empty)", list4);

    // --- Skip List Specifics ---
    std::cout << "\n--- Skip List Specifics ---" << std::endl;
    Container<int> skip_test_list;
    std::cout << "Adding elements for skip list test..." << std::endl;
    for (int i = 0; i < 20; ++i) {
        skip_test_list.push_back(i * 3);
    }
    print_container("skip_test_list", skip_test_list);

    int search_val1 = 15;
    std::cout << "Contains " << search_val1 << "? " << (skip_test_list.contains(search_val1) ? "Yes" : "No") << std::endl;
    int search_val2 = 100;
    std::cout << "Contains " << search_val2 << "? " << (skip_test_list.contains(search_val2) ? "Yes" : "No") << std::endl;

    auto found_it = skip_test_list.find(30);
    if (found_it != skip_test_list.end()) {
        std::cout << "Found 30 at value: " << *found_it << std::endl;
        skip_test_list.erase(found_it);
        print_container("skip_test_list after erasing 30", skip_test_list);
    } else {
        std::cout << "30 not found." << std::endl;
    }

    found_it = skip_test_list.find(30); // Try to find 30 again
    std::cout << "Contains 30 after erase? " << (skip_test_list.contains(30) ? "Yes" : "No") << std::endl;

    std::cout << "\n--- Error Handling Test ---" << std::endl;
    Container<int> empty_list;
    try {
        empty_list.front();
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    try {
        empty_list.pop_back();
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    // Test with initializer list
    Container<std::string> str_list = {"hello", "world", "cpp", "container"};
    print_container("str_list (initializer_list)", str_list);

    // Test with default constructor and count
    Container<double> double_list(3); // 3 default-constructed doubles
    print_container("double_list (count constructor)", double_list);

    // Test with count and value
    Container<char> char_list(5, 'X');
    print_container("char_list (count, value constructor)", char_list);


    return 0;
}