cmake_minimum_required(VERSION 3.21)
project(pebble LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(libs/primitives)
add_subdirectory(libs/stats)
add_subdirectory(libs/isa)
