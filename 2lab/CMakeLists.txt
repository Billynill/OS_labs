cmake_minimum_required(VERSION 3.10)
project(2lab C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(2lab dice_sim.c)

find_package(Threads REQUIRED)
target_link_libraries(2lab Threads::Threads)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(2lab rt)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
target_compile_options(2lab PRIVATE -O2 -Wall)