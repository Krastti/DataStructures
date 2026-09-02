cmake_minimum_required(VERSION 4.2)
project(DataStructures)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


# Бинарники

add_executable(DataStructures
        main.cpp
)

add_library(BinaryTree INTERFACE
        include/Trees/BinaryTree.h
        include/Trees/BinaryTree.tpp
        include/Trees/RBTree.h
        include/Trees/RBTree.tpp
)

add_library(Stack INTERFACE
        include/Stack/stack.h
        include/Stack/stack.tpp
)

add_library(SmartPointer INTERFACE
        include/SmartPointers/UniquePtr.h
        include/SmartPointers/UniquePtr.tpp
        include/SmartPointers/SharedPtr.h
        include/SmartPointers/SharedPtr.tpp
)

target_link_libraries(DataStructures
        BinaryTree
        Stack
        SmartPointer
)

# Тесты

enable_testing()

add_executable(UnitTests
        tests/SmartPointer/SharedPtrTests.cpp
        tests/SmartPointer/UniquePtrTests.cpp
)

target_link_libraries(UnitTests PRIVATE
        SmartPointer
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(UnitTests)
