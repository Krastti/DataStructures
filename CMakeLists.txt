cmake_minimum_required(VERSION 3.20)
project(DataStructures)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(GTest QUIET)
if (NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()


# Бинарники

add_executable(DataStructures
        main.cpp
        include/SecondLab/AlphabeticalIndex.cpp
        include/SecondLab/PersonIndex.cpp
)
target_include_directories(DataStructures PRIVATE include)

add_library(Sequence INTERFACE
        include/Sequence/Sequence.h
        include/Sequence/ArraySequence.h
        include/Sequence/ArraySequence.tpp
)

add_library(BinaryTree INTERFACE
        include/Trees/BinaryTree.h
        include/Trees/BinaryTree.tpp
        include/Trees/RedBlackTree.h
        include/Trees/RedBlackTree.tpp
        include/Trees/Set.h
        include/Trees/Set.tpp
)

add_library(Stack INTERFACE
        include/Stack/stack.h
        include/Stack/stack.tpp
)

add_library(Array INTERFACE
        include/Array/Array.h
        include/Array/Array.tpp
)

add_library(SmartPointer INTERFACE
        include/SmartPointers/UniquePtr.h
        include/SmartPointers/UniquePtr.tpp
        include/SmartPointers/SharedPtr.h
        include/SmartPointers/SharedPtr.tpp
)

add_library(Graph INTERFACE
        include/Optional/Optional.h
        include/Optional/Optional.tpp
        include/Graph/Graph.h
        include/Graph/AdjacencyGraph.h
        include/Graph/AdjacencyGraph.tpp
        include/Graph/ShortestPath.h
        include/Graph/ShortestPath.tpp
)

target_link_libraries(DataStructures
        Sequence
        BinaryTree
        Stack
        Array
        SmartPointer
        Graph
)

# Тесты

enable_testing()

add_executable(UnitTests
        tests/SmartPointers/SharedPtrTests.cpp
        tests/SmartPointers/UniquePtrTests.cpp
        tests/Array/ArrayTests.cpp
        tests/Trees/BinaryTreeTests.cpp
        tests/SecondLab/AlphabeticalIndexTests.cpp
        tests/ThirdLab/ShortestPathTests.cpp
        include/SecondLab/AlphabeticalIndex.cpp
        include/SecondLab/PersonIndex.cpp
        tests/Timer.h
    )

target_link_libraries(UnitTests PRIVATE
        SmartPointer
        BinaryTree
        Graph
        Array
        GTest::gtest_main
)
target_include_directories(UnitTests PRIVATE include)

include(GoogleTest)
gtest_discover_tests(UnitTests)
