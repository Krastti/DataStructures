cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

project(DataStructures)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

if(BUILD_TESTING)
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
        include/Trees/BinaryTreeBase.h
        include/Trees/BinaryTreeBase.tpp
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

# Общая реализация для CLI и тестов интерпретатора.
add_library(MiniLangCore STATIC
        interpreter/sources/lexer.cpp
        interpreter/sources/parser.cpp
        interpreter/headers/expressionTree.h
        interpreter/sources/expressionTree.cpp
        interpreter/sources/ast.cpp
        interpreter/sources/interpreter.cpp
)
target_include_directories(MiniLangCore PUBLIC interpreter/headers)
target_link_libraries(MiniLangCore PUBLIC Array BinaryTree SmartPointer)

add_executable(MiniLangCpp interpreter/main.cpp)
target_link_libraries(MiniLangCpp PRIVATE MiniLangCore)

# Тесты

if(BUILD_TESTING)
  add_executable(UnitTests
          tests/Interpreter/LexerTests.cpp
          tests/Interpreter/ExpressionTreeTests.cpp
          tests/Interpreter/ParserTests.cpp
          tests/Interpreter/InterpreterTests.cpp
          tests/SmartPointers/SharedPtrTests.cpp
          tests/SmartPointers/UniquePtrTests.cpp
          tests/SmartPointers/SmartPointerSafetyTests.cpp
          tests/Array/ArrayTests.cpp
          tests/Trees/BinaryTreeTests.cpp
          tests/SecondLab/AlphabeticalIndexTests.cpp
          tests/ThirdLab/ShortestPathTests.cpp
          include/SecondLab/AlphabeticalIndex.cpp
          include/SecondLab/PersonIndex.cpp
          tests/Timer.h
      )

  find_package(Threads REQUIRED)

  target_link_libraries(UnitTests PRIVATE
          MiniLangCore
          Threads::Threads
          SmartPointer
          BinaryTree
          Graph
          Array
          GTest::gtest_main
  )
  target_include_directories(UnitTests PRIVATE include)

  include(GoogleTest)
  gtest_discover_tests(UnitTests)

  add_executable(TreeSafetyTests tests/Trees/TreeSafetyTests.cpp)
  target_link_libraries(TreeSafetyTests PRIVATE BinaryTree MiniLangCore GTest::gtest_main)
  target_include_directories(TreeSafetyTests PRIVATE include)
  gtest_discover_tests(TreeSafetyTests)

  add_executable(SharedPtrAllocationTests tests/SmartPointers/SharedPtrAllocationTests.cpp)
  target_link_libraries(SharedPtrAllocationTests PRIVATE SmartPointer GTest::gtest_main)
  target_include_directories(SharedPtrAllocationTests PRIVATE include)
  gtest_discover_tests(SharedPtrAllocationTests)

  add_test(NAME MiniLangCliTests
    COMMAND ${CMAKE_COMMAND}
      "-DMINILANG_EXECUTABLE=$<TARGET_FILE:MiniLangCpp>"
      "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/minilang-cli-tests"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/Interpreter/CliTests.cmake"
  )
endif()

# Графический интерфейс собирается отдельно от консольной версии.
option(BUILD_GUI "Build Qt 6 Widgets application" OFF)
if(BUILD_GUI)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Concurrent)
  add_library(DataStructuresGuiWidgets STATIC
      app/gui/AlphabeticalIndexWidget.cpp
      app/gui/PersonIndexWidget.cpp
      app/gui/GraphWidget.cpp
      app/gui/GraphScene.cpp
      include/SecondLab/AlphabeticalIndex.cpp
      include/SecondLab/PersonIndex.cpp
  )
  target_include_directories(DataStructuresGuiWidgets PUBLIC include app/gui)
  target_link_libraries(DataStructuresGuiWidgets PUBLIC Qt6::Widgets Qt6::Concurrent)
  add_executable(DataStructuresGui app/gui/main.cpp)
  target_link_libraries(DataStructuresGui PRIVATE DataStructuresGuiWidgets)
  if(BUILD_TESTING)
    add_executable(GuiTests tests/Gui/GuiTests.cpp)
    target_link_libraries(GuiTests PRIVATE DataStructuresGuiWidgets GTest::gtest)
    add_test(NAME GuiTests COMMAND GuiTests)
    set_tests_properties(GuiTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
  endif()
endif()
