cmake_minimum_required(VERSION 3.10)
project(dig1024)

set(CMAKE_CXX_STANDARD 17)

include_directories(./567lib)
include_directories(./)

add_executable(dig1024
        main.cpp
)

target_link_libraries(dig1024 pthread curl)