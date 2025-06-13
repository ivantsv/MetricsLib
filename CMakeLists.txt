cmake_minimum_required(VERSION 3.10)
project(metrics VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(bin)
add_subdirectory(tests)

enable_testing()
