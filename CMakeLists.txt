cmake_minimum_required(VERSION 3.28)
project(list_container)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найдём GoogleTest
find_package(GTest REQUIRED)

# Основной исполняемый файл
add_executable(list_container main.cpp)

# Тестовый исполняемый файл
add_executable(test test.cpp)
target_link_libraries(test PRIVATE GTest::gtest GTest::gtest_main)

# Включаем автоматический запуск тестов
include(GoogleTest)
gtest_discover_tests(test)
