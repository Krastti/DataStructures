cmake_minimum_required(VERSION 4.2)
project(DataStructures)

set(CMAKE_CXX_STANDARD 20)

add_executable(DataStructures
        main.cpp
        BinaryTree/BinaryTree.h
)

add_library(Stack INTERFACE
        Stack/stack.h
        Stack/stack.tpp
)

target_link_libraries(DataStructures
        Stack
)
