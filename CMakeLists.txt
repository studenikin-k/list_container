cmake_minimum_required(VERSION 3.16)

project(list_container CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(CONTAINER_SOURCES
        src/container/container.cpp
        src/container/dll_ops/dll_ops.cpp
        src/container/helpers/helpers.cpp
        src/container/skip_ops/skip_ops.cpp
)


add_executable(list_container
        main.cpp
        ${CONTAINER_SOURCES}
)

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