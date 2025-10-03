cmake_minimum_required(VERSION 3.10)
project(Lab1_OS C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Включаем отладочную информацию и предупреждения
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# Основная программа (родительский процесс)
add_executable(parent src/parent.c)

# Дочерняя программа
add_executable(child src/child.c)

# Указываем, что для сборки родителя нужен child
add_dependencies(parent child)

# Устанавливаем выходные файлы в корневую директорию сборки
set_target_properties(parent child PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)