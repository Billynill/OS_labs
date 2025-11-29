cmake_minimum_required(VERSION 3.10)
project(OS_Lab2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Настройки компиляции
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Для macOS
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dylib")

# Поиск необходимых библиотек
find_library(M_LIB m)
find_library(DL_LIB dl)

# Создание динамических библиотек
add_library(method1 SHARED method1.c)
add_library(method2 SHARED method2.c)

# Связывание с математической библиотекой
target_link_libraries(method1 ${M_LIB})
target_link_libraries(method2 ${M_LIB})

# Создание исполняемых файлов
add_executable(program1 program1.c)
add_executable(program2 program2.c)

# Связывание program1 с method1 во время компиляции
target_link_libraries(program1 method1 ${M_LIB})

# Связывание program2 с библиотекой dl для динамической загрузки
if(DL_LIB)
    target_link_libraries(program2 ${DL_LIB})
endif()

# Копирование библиотек в директорию сборки
add_custom_command(TARGET program2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:method1>
        $<TARGET_FILE:method2>
        $<TARGET_FILE_DIR:program2>
)

message(STATUS "Библиотеки будут созданы с расширением .dylib")