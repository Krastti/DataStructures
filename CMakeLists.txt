cmake_minimum_required(VERSION 4.2)
project(DataStructures)

set(CMAKE_CXX_STANDARD 20)

add_executable(DataStructures
        main.cpp
        include/Trees/BinaryTree.h
        include/Trees/RBTree.h
)

add_library(Stack INTERFACE
        include/Stack/stack.h
        include/Stack/stack.tpp
)

add_library(SmartPointer INTERFACE
        include/SmartPointer/UniquePtr.h
        include/SmartPointer/UniquePtr.tpp
        include/SmartPointer/SharedPtr.h
        include/SmartPointer/SharedPtr.tpp
)

target_link_libraries(DataStructures
        Stack
)

