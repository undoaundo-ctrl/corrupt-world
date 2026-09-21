cmake_minimum_required(VERSION 3.10)
project(corrupt_world C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(corrupt_world src/main.c)

target_link_libraries(corrupt_world
    glfw
    OpenGL::GL
    OpenGL::GLU
    m
)
