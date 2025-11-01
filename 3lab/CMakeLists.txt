cmake_minimum_required(VERSION 4.0)
project(3lab C)

set(CMAKE_C_STANDARD 11)

add_executable(server server.c)

add_executable(client client.c)
