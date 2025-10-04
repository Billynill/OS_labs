cmake_minimum_required(VERSION 3.10)
project(Lab1_OS C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

add_executable(parent src/parent.c)

add_executable(child src/child.c)

add_dependencies(parent child)

set_target_properties(parent child PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)