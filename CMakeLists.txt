cmake_minimum_required(VERSION 3.16)

project(list_container CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(APP_SOURCES
        src/main.cpp
)


add_executable(list_container ${APP_SOURCES})


target_include_directories(list_container PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(list_container PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -fdiagnostics-color=always
    )
endif()


find_package(GTest CONFIG REQUIRED)


add_executable(list_container_tests test/test.cpp)


target_include_directories(list_container_tests PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include

)


target_link_libraries(list_container_tests PUBLIC
        GTest::gtest_main
)


if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(list_container_tests PRIVATE
            -Wall
            -Wextra
            -Wpedantic

            -fdiagnostics-color=always
    )
endif()


enable_testing()
add_test(NAME run_list_container_tests COMMAND list_container_tests)