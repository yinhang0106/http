cmake_minimum_required(VERSION 3.22)

project(http LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(extern/fmt)

add_subdirectory(http)
add_subdirectory(tests)
