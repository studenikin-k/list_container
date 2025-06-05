cmake_minimum_required(VERSION 3.16)

project(list_container CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Определяем исходные файлы для основного приложения
set(APP_SOURCES
        src/main.cpp
)

# Добавляем исполняемый файл для основного приложения
add_executable(list_container ${APP_SOURCES})

# Указываем директории для поиска заголовочных файлов (include/)
target_include_directories(list_container PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Компиляторные опции для основного приложения
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(list_container PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -fdiagnostics-color=always
    )
endif()

# --- Подключение Google Test ---
# Сначала находим пакет GTest
find_package(GTest CONFIG REQUIRED) # Ищем GTest через его конфигурационный файл

# Теперь, после того как GTest найден, мы можем объявить наши тесты.
# Добавляем исполняемый файл для тестов
# test.cpp находится в корне проекта согласно вашей структуре на фото.
add_executable(list_container_tests test/test.cpp)

# Указываем директории для поиска заголовочных файлов для тестов.
# Тесты должны видеть наш include/
target_include_directories(list_container_tests PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        # Дополнительные include-директории для GTest, если find_package их не добавил
        # Но с GTest::gtest_main обычно этого не требуется, пути к заголовкам GTest
        # автоматически управляются целевым объектом GTest::gtest_main.
)

# Связываем тесты с библиотеками GTest
# GTest::gtest_main предоставляет точку входа main() для тестов.
# Мы не линкуем list_container, так как все в .h файлах.
target_link_libraries(list_container_tests PUBLIC
        GTest::gtest_main
)

# Компиляторные опции для тестов (можно скопировать или настроить отдельно)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(list_container_tests PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            # -Werror # Часто отключают для тестов, чтобы ошибки в тестах не останавливали сборку
            -fdiagnostics-color=always
    )
endif()

# Включаем обнаружение тестов CTest (для запуска тестов через CTest)
enable_testing()
add_test(NAME run_list_container_tests COMMAND list_container_tests)