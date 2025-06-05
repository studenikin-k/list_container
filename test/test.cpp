#include "gtest/gtest.h" // Подключаем заголовок Google Test
#include "container/container.h" // Подключаем заголовок вашего контейнера
#include <string>
#include <vector>
#include <stdexcept> // Для проверки исключений
#include <algorithm> // Для std::equal, std::sort и т.д. (если нужны)
#include <list> // Для сравнения, если необходимо

// --- 1. Тесты конструкторов и деструктора ---
TEST(ContainerConstructorsTest, DefaultConstructor) {
    Container<int> c;
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
}

TEST(ContainerConstructorsTest, CountConstructor) {
    Container<int> c(5); // 5 default-constructed ints (0s)
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(c.front(), 0); // Все должны быть 0 (для int)
        c.pop_front();
    }
    EXPECT_TRUE(c.empty());

    Container<std::string> cs(3); // 3 empty strings
    EXPECT_EQ(cs.size(), 3);
    EXPECT_EQ(cs.front(), "");
    EXPECT_EQ(cs.back(), "");
}

TEST(ContainerConstructorsTest, CountValueConstructor) {
    Container<int> c(5, 10); // 5 elements with value 10
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(c.front(), 10);
        c.pop_front();
    }
    EXPECT_TRUE(c.empty());

    Container<char> cc(3, 'X');
    EXPECT_EQ(cc.size(), 3);
    EXPECT_EQ(cc.front(), 'X');
    EXPECT_EQ(cc.back(), 'X');
}

TEST(ContainerConstructorsTest, RangeConstructor) {
    std::vector<int> v = {1, 2, 3, 4, 5};
    Container<int> c(v.begin(), v.end()); // Эта строка 34 из вашего лога, она верна
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 5);
    int expected = 1;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected++);
    }
}

TEST(ContainerConstructorsTest, InitializerListConstructor) {
    Container<int> c = {10, 20, 30, 40};
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 4);
    auto it = c.begin();
    EXPECT_EQ(*it++, 10);
    EXPECT_EQ(*it++, 20);
    EXPECT_EQ(*it++, 30);
    EXPECT_EQ(*it++, 40);
    EXPECT_EQ(it, c.end());
}

TEST(ContainerConstructorsTest, CopyConstructor) {
    Container<int> original = {1, 2, 3};
    Container<int> copied = original; // Copy
    EXPECT_EQ(copied.size(), original.size());
    EXPECT_FALSE(copied.empty());

    // Проверяем содержимое
    auto it_orig = original.begin();
    auto it_copy = copied.begin();
    while(it_orig != original.end()) {
        EXPECT_EQ(*it_orig++, *it_copy++);
    }
    EXPECT_EQ(it_copy, copied.end());

    // Убеждаемся, что это глубокая копия
    original.clear();
    EXPECT_TRUE(original.empty());
    EXPECT_FALSE(copied.empty()); // Копированный контейнер должен быть нетронут
    EXPECT_EQ(copied.size(), 3);
}

TEST(ContainerConstructorsTest, MoveConstructor) {
    Container<int> original = {1, 2, 3};
    Container<int> moved = std::move(original); // Move
    EXPECT_FALSE(moved.empty());
    EXPECT_EQ(moved.size(), 3);

    // Original должен быть в валидном, но пустом состоянии
    EXPECT_TRUE(original.empty());
    EXPECT_EQ(original.size(), 0); // Проверяем, что размер сброшен (ваш initialize_container это делает)

    // Проверяем содержимое moved
    auto it = moved.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, moved.end());
}

// --- 2. Тесты операторов присваивания ---
TEST(ContainerAssignmentTest, CopyAssignment) {
    Container<int> c1 = {1, 2, 3};
    Container<int> c2 = {10, 20};
    c2 = c1; // Copy assignment
    EXPECT_EQ(c2.size(), c1.size());
    EXPECT_FALSE(c2.empty());

    // Проверяем содержимое
    auto it1 = c1.begin();
    auto it2 = c2.begin();
    while(it1 != c1.end()) {
        EXPECT_EQ(*it1++, *it2++);
    }
    EXPECT_EQ(it2, c2.end());

    c1.clear(); // Проверяем глубокую копию
    EXPECT_FALSE(c2.empty());
    EXPECT_EQ(c2.size(), 3); // Должен остаться 3 элемента
}

TEST(ContainerAssignmentTest, MoveAssignment) {
    Container<int> c1 = {1, 2, 3};
    Container<int> c2 = {10, 20};
    c2 = std::move(c1); // Move assignment
    EXPECT_FALSE(c2.empty());
    EXPECT_EQ(c2.size(), 3);

    // c1 должен быть в валидном, но пустом состоянии
    EXPECT_TRUE(c1.empty());
    EXPECT_EQ(c1.size(), 0);

    // Проверяем содержимое c2
    auto it = c2.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(it, c2.end());
}

TEST(ContainerAssignmentTest, InitializerListAssignment) {
    Container<int> c = {100, 200, 300};
    c = {1, 2, 3, 4}; // Assign from initializer list
    EXPECT_EQ(c.size(), 4);

    // Проверяем порядок после присваивания
    auto it = c.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(it, c.end());
}

// --- 3. Тесты итераторов ---
TEST(ContainerIteratorsTest, ForwardTraversal) {
    Container<int> c = {10, 20, 30, 40};
    std::vector<int> expected = {10, 20, 30, 40};
    size_t i = 0;
    for (auto it = c.begin(); it != c.end(); ++it, ++i) {
        EXPECT_EQ(*it, expected[i]);
    }
    EXPECT_EQ(i, expected.size());
}

TEST(ContainerIteratorsTest, BackwardTraversal) {
    Container<int> c = {10, 20, 30, 40};
    std::vector<int> expected = {40, 30, 20, 10};
    size_t i = 0;

    // Итерация назад от end()
    for (auto it = c.end(); it != c.begin();) {
        --it; // Сначала декрементируем, чтобы получить указатель на фактический элемент
        EXPECT_EQ(*it, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());
}

TEST(ContainerIteratorsTest, ConstIterators) {
    const Container<int> c = {1, 2, 3};
    auto it_cbegin = c.cbegin();
    EXPECT_EQ(*it_cbegin, 1);

    // Проверяем, что нельзя изменить значение через const_iterator
    // *it_cbegin = 100; // Это должно вызывать ошибку компиляции (раскомментируйте для проверки)

    // Проверяем const_iterator, полученный из begin()
    auto it_begin_const = c.begin();
    EXPECT_EQ(*it_begin_const, 1);
}

// --- 4. Тесты доступа к элементам ---
TEST(ContainerAccessTest, FrontAndBack) {
    Container<int> c = {5, 10, 15};
    EXPECT_EQ(c.front(), 5);
    EXPECT_EQ(c.back(), 15);

    c.push_front(0);
    EXPECT_EQ(c.front(), 0);
    EXPECT_EQ(c.back(), 15);

    c.pop_back();
    EXPECT_EQ(c.front(), 0);
    EXPECT_EQ(c.back(), 10);
    EXPECT_EQ(c.size(), 3); // 0, 5, 10
}

TEST(ContainerAccessTest, EmptyContainerFrontBackThrows) {
    Container<int> c;
    EXPECT_THROW(c.front(), std::out_of_range);
    EXPECT_THROW(c.back(), std::out_of_range);
}

// --- 5. Тесты вместимости ---
TEST(ContainerCapacityTest, EmptyAndSize) {
    Container<int> c;
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);

    c.push_back(1);
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 1);

    c.push_back(2);
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 2);

    c.pop_front();
    EXPECT_FALSE(c.empty());
    EXPECT_EQ(c.size(), 1);

    c.pop_back();
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
}

// --- 6. Тесты модификаторов ---
TEST(ContainerModifiersTest, PushBack) {
    Container<int> c;
    c.push_back(1);
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.back(), 1);
    EXPECT_EQ(c.front(), 1);
    c.push_back(2); // Теперь {1, 2}
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(c.back(), 2);
    EXPECT_EQ(c.front(), 1);
    c.push_back(0); // Теперь {0, 1, 2} (Skip List сохраняет порядок)
    EXPECT_EQ(c.size(), 3);
    EXPECT_EQ(c.front(), 0);
    EXPECT_EQ(c.back(), 2);
}

TEST(ContainerModifiersTest, PushFront) {
    Container<int> c;
    c.push_front(1);
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.front(), 1);
    EXPECT_EQ(c.back(), 1);
    c.push_front(2); // Теперь {1, 2}
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(c.front(), 1); // Skip List сохраняет порядок
    EXPECT_EQ(c.back(), 2);
    c.push_front(0); // Теперь {0, 1, 2}
    EXPECT_EQ(c.size(), 3);
    EXPECT_EQ(c.front(), 0);
    EXPECT_EQ(c.back(), 2);
}

TEST(ContainerModifiersTest, PopBack) {
    Container<int> c = {1, 2, 3};
    c.pop_back(); // удаляет 3
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(c.back(), 2);
    EXPECT_EQ(c.front(), 1);
    c.pop_back(); // удаляет 2
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.back(), 1);
    c.pop_back(); // удаляет 1
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
    EXPECT_THROW(c.pop_back(), std::out_of_range); // Проверка на пустой контейнер
}

TEST(ContainerModifiersTest, PopFront) {
    Container<int> c = {1, 2, 3};
    c.pop_front(); // удаляет 1
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(c.front(), 2);
    EXPECT_EQ(c.back(), 3);
    c.pop_front(); // удаляет 2
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.front(), 3);
    c.pop_front(); // удаляет 3
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
    EXPECT_THROW(c.pop_front(), std::out_of_range); // Проверка на пустой контейнер
}

TEST(ContainerModifiersTest, Clear) {
    Container<int> c = {1, 2, 3, 4, 5};
    EXPECT_FALSE(c.empty());
    c.clear();
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
}

TEST(ContainerModifiersTest, InsertSingleElement) {
    Container<int> c = {10, 30, 40};
    auto it_pos = c.find(30); // Insert before 30 (DLL hint), Skip List puts it by value
    c.insert(it_pos, 20);

    EXPECT_EQ(c.size(), 4);
    std::vector<int> expected = {10, 20, 30, 40};
    size_t i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());


    // Insert at beginning (logically, as 5 < 10)
    c.insert(c.begin(), 5);
    EXPECT_EQ(c.size(), 5);
    expected = {5, 10, 20, 30, 40};
    i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }

    // Insert at end (logically, as 45 > 40)
    c.insert(c.end(), 45);
    EXPECT_EQ(c.size(), 6);
    expected = {5, 10, 20, 30, 40, 45};
    i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }
}

TEST(ContainerModifiersTest, InsertMoveElement) {
    Container<std::string> c = {"b", "d"};
    std::string s = "c";
    c.insert(c.find("d"), std::move(s));
    EXPECT_EQ(c.size(), 3);
    std::vector<std::string> expected = {"b", "c", "d"};
    size_t i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());
    EXPECT_TRUE(s.empty()); // s должен быть перемещен и пуст
}

TEST(ContainerModifiersTest, InsertCountElements) {
    Container<int> c = {10, 50};
    auto it_pos = c.find(50); // Hint: before 50
    c.insert(it_pos, 3, 20); // Insert 3x 20s. Skip List will order them {10, 20, 20, 20, 50}

    EXPECT_EQ(c.size(), 5);
    std::vector<int> expected = {10, 20, 20, 20, 50};
    size_t i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());

    // Insert 0 elements
    Container<int> c_empty_insert = {1,2,3};
    auto it_ret = c_empty_insert.insert(c_empty_insert.begin(), 0, 99);
    EXPECT_EQ(c_empty_insert.size(), 3);
    EXPECT_EQ(it_ret, c_empty_insert.begin());
}





TEST(ContainerModifiersTest, EraseSingleElement) {
    Container<int> c = {10, 20, 30, 40};
    auto it_erase = c.find(20);
    ASSERT_NE(it_erase, c.end()); // Убеждаемся, что элемент найден

    // Erase 20, returns iterator to 30
    auto it_after_erase = c.erase(it_erase);
    EXPECT_EQ(*it_after_erase, 30); // Check returned iterator

    EXPECT_EQ(c.size(), 3);
    std::vector<int> expected = {10, 30, 40};
    size_t i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());

    // Erase front
    it_after_erase = c.erase(c.begin()); // Erase 10, returns iterator to 30
    EXPECT_EQ(c.size(), 2);
    EXPECT_EQ(*it_after_erase, 30); // Now 30 is front
    EXPECT_EQ(c.front(), 30);

    // Erase back
    it_after_erase = c.erase(c.find(40)); // Erase 40, returns iterator to end()
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.back(), 30);
    EXPECT_EQ(it_after_erase, c.end());

    it_after_erase = c.erase(c.begin()); // Erase last element (30), returns iterator to end()
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
    EXPECT_EQ(it_after_erase, c.end()); // Should return end() for last element erase
}

TEST(ContainerModifiersTest, EraseRange) {
    Container<int> c = {1, 2, 3, 4, 5, 6, 7};
    auto it_begin = c.find(3);
    auto it_end = c.find(6); // This erases 3, 4, 5
    ASSERT_NE(it_begin, c.end());
    ASSERT_NE(it_end, c.end());

    // Erase [3, 6) -> 3, 4, 5. Returns iterator to 6.
    auto it_ret = c.erase(it_begin, it_end);
    EXPECT_EQ(c.size(), 4);
    EXPECT_EQ(*it_ret, 6); // Check returned iterator

    std::vector<int> expected = {1, 2, 6, 7};
    size_t i = 0;
    for (const auto& val : c) {
        EXPECT_EQ(val, expected[i++]);
    }
    EXPECT_EQ(i, expected.size());

    // Erase all
    it_ret = c.erase(c.begin(), c.end());
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
    EXPECT_EQ(it_ret, c.end()); // Should return end()
}

TEST(ContainerModifiersTest, Swap) {
    Container<int> c1 = {1, 2, 3};
    Container<int> c2 = {10, 20, 30, 40};

    c1.swap(c2);

    // c1 теперь содержит элементы c2
    EXPECT_EQ(c1.size(), 4);
    EXPECT_EQ(c1.front(), 10);
    EXPECT_EQ(c1.back(), 40);
    std::vector<int> expected1 = {10, 20, 30, 40};
    EXPECT_TRUE(std::equal(c1.begin(), c1.end(), expected1.begin()));

    // c2 теперь содержит элементы c1
    EXPECT_EQ(c2.size(), 3);
    EXPECT_EQ(c2.front(), 1);
    EXPECT_EQ(c2.back(), 3);
    std::vector<int> expected2 = {1, 2, 3};
    EXPECT_TRUE(std::equal(c2.begin(), c2.end(), expected2.begin()));

    // Проверка не-член swap
    Container<int> c3 = {11, 22};
    Container<int> c4 = {33, 44, 55};
    swap(c3, c4);
    EXPECT_EQ(c3.size(), 3);
    EXPECT_EQ(c4.size(), 2);
    EXPECT_EQ(c3.front(), 33);
    EXPECT_EQ(c4.front(), 11);
}


// --- 7. Тесты Skip List специфических операций ---
TEST(ContainerSkipListTest, Find) {
    Container<int> c;
    for (int i = 0; i < 100; i += 5) { // 0, 5, 10, ..., 95
        c.push_back(i);
    }

    auto it_found = c.find(25);
    ASSERT_NE(it_found, c.end());
    EXPECT_EQ(*it_found, 25);

    it_found = c.find(95);
    ASSERT_NE(it_found, c.end());
    EXPECT_EQ(*it_found, 95);

    it_found = c.find(0);
    ASSERT_NE(it_found, c.end());
    EXPECT_EQ(*it_found, 0);

    auto it_not_found = c.find(101); // Value not in container
    EXPECT_EQ(it_not_found, c.end());

    it_not_found = c.find(23); // Value not in container
    EXPECT_EQ(it_not_found, c.end());

    // Find in empty container
    Container<int> empty_c;
    EXPECT_EQ(empty_c.find(10), empty_c.end());
}

TEST(ContainerSkipListTest, ConstFind) {
    const Container<int> c = {10, 20, 30, 40, 50};
    auto it_found = c.find(30);
    ASSERT_NE(it_found, c.cend());
    EXPECT_EQ(*it_found, 30);

    auto it_not_found = c.find(35);
    EXPECT_EQ(it_not_found, c.cend());
}


TEST(ContainerSkipListTest, Contains) {
    Container<int> c;
    for (int i = 0; i < 100; i += 5) {
        c.push_back(i);
    }

    EXPECT_TRUE(c.contains(0));
    EXPECT_TRUE(c.contains(50));
    EXPECT_TRUE(c.contains(95));
    EXPECT_FALSE(c.contains(1));
    EXPECT_FALSE(c.contains(99));
    EXPECT_FALSE(c.contains(100));

    // Contains in empty container
    Container<int> empty_c;
    EXPECT_FALSE(empty_c.contains(10));
}

// Дополнительные тесты для edge-кейсов и производительности
TEST(ContainerEdgeCasesTest, InsertIntoEmpty) {
    Container<int> c;
    c.insert(c.begin(), 5);
    EXPECT_EQ(c.size(), 1);
    EXPECT_EQ(c.front(), 5);
    EXPECT_EQ(c.back(), 5);
}

TEST(ContainerEdgeCasesTest, EraseLastElement) {
    Container<int> c = {42};
    c.erase(c.begin());
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0);
    EXPECT_EQ(c.begin(), c.end());
}

TEST(ContainerEdgeCasesTest, EraseOnlyElementInRange) {
    Container<int> c = {10, 20, 30};
    c.erase(c.find(20), c.find(30)); // Erase 20
    EXPECT_EQ(c.size(), 2);
    std::vector<int> expected = {10, 30};
    EXPECT_TRUE(std::equal(c.begin(), c.end(), expected.begin()));
}

// Тест на вставку дубликатов (Skip List должен их корректно обрабатывать)
TEST(ContainerSkipListTest, InsertDuplicates) {
    Container<int> c;
    c.insert(c.end(), 20);
    c.insert(c.end(), 10);
    c.insert(c.end(), 20); // Duplicate
    c.insert(c.end(), 30);
    c.insert(c.end(), 10); // Duplicate

    EXPECT_EQ(c.size(), 5);
    std::vector<int> expected = {10, 10, 20, 20, 30};
    EXPECT_TRUE(std::equal(c.begin(), c.end(), expected.begin()));
}

// Тест на удаление дубликатов (find найдет первый, удалит его)
TEST(ContainerSkipListTest, EraseDuplicates) {
    Container<int> c = {10, 20, 10, 30, 20, 10}; // {10, 10, 10, 20, 20, 30}
    EXPECT_EQ(c.size(), 6);

    c.erase(c.find(10)); // Удалит первый 10
    EXPECT_EQ(c.size(), 5);
    std::vector<int> expected1 = {10, 10, 20, 20, 30};
    EXPECT_TRUE(std::equal(c.begin(), c.end(), expected1.begin()));

    c.erase(c.find(20)); // Удалит первый 20
    EXPECT_EQ(c.size(), 4);
    std::vector<int> expected2 = {10, 10, 20, 30};
    EXPECT_TRUE(std::equal(c.begin(), c.end(), expected2.begin()));
}