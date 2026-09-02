cmake_minimum_required(VERSION 4.2)
project(DataStructures)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


# Бинарники

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

# Тесты

enable_testing()

add_executable(UnitTests
        tests/SmartPointer/SharedPtrTests.cpp
        tests/SmartPointer/UniquePtrTests.cpp
)

include(GoogleTest)
gtest_discover_tests(UnitTests)
