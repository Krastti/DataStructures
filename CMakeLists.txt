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

add_library(Sequence INTERFACE
        DataStructures/Sequence/Sequence.h
        DataStructures/Sequence/ArraySequence.h
        DataStructures/Sequence/ArraySequence.tpp
)

add_library(BinaryTree INTERFACE
        DataStructures/Trees/BinaryTreeBase.h
        DataStructures/Trees/BinaryTreeBase.tpp
        DataStructures/Trees/BinaryTree.h
        DataStructures/Trees/BinaryTree.tpp
        DataStructures/Trees/RedBlackTree.h
        DataStructures/Trees/RedBlackTree.tpp
        DataStructures/Trees/Set.h
        DataStructures/Trees/Set.tpp
)

add_library(Stack INTERFACE
        DataStructures/Stack/stack.h
        DataStructures/Stack/stack.tpp
)

add_library(Array INTERFACE
        DataStructures/Array/Array.h
        DataStructures/Array/Array.tpp
)

add_library(SmartPointer INTERFACE
        SmartPointers/include/UniquePtr.h
        SmartPointers/include/UniquePtr.tpp
        SmartPointers/include/SharedPtr.h
        SmartPointers/include/SharedPtr.tpp
)

add_library(Optional INTERFACE
        DataStructures/Optional/Optional.h
        DataStructures/Optional/Optional.tpp
)

add_library(Graph INTERFACE
        DataStructures/Graph/Graph.h
        DataStructures/Graph/AdjacencyGraph.h
        DataStructures/Graph/AdjacencyGraph.tpp
)

add_library(ThirdLabPaths INTERFACE
        ThirdLab/include/ShortestPath.h
        ThirdLab/include/ShortestPath.tpp
)

foreach(module IN ITEMS Array Sequence Stack BinaryTree Optional Graph)
  target_include_directories(${module} INTERFACE DataStructures)
endforeach()
target_include_directories(SmartPointer INTERFACE SmartPointers/include)

target_link_libraries(BinaryTree INTERFACE Array Stack)
target_link_libraries(Graph INTERFACE Array BinaryTree)
target_include_directories(ThirdLabPaths INTERFACE ThirdLab/include)
target_link_libraries(ThirdLabPaths INTERFACE Graph Optional)

add_library(SecondLabIndexes STATIC
        SecondLab/source/AlphabeticalIndex.cpp
        SecondLab/source/PersonIndex.cpp
)
target_include_directories(SecondLabIndexes PUBLIC SecondLab/include)
target_link_libraries(SecondLabIndexes PUBLIC Array BinaryTree SmartPointer)

add_executable(DataStructures main.cpp)
target_link_libraries(DataStructures PRIVATE SecondLabIndexes ThirdLabPaths Array)

# Общая реализация для CLI и тестов интерпретатора.
add_library(MiniLangCore STATIC
        Interpreter/sources/lexer.cpp
        Interpreter/sources/parser.cpp
        Interpreter/headers/expressionTree.h
        Interpreter/sources/expressionTree.cpp
        Interpreter/sources/ast.cpp
        Interpreter/sources/interpreter.cpp
)
target_include_directories(MiniLangCore PUBLIC Interpreter/headers)
target_link_libraries(MiniLangCore PUBLIC Array BinaryTree Optional SmartPointer)

add_executable(MiniLangCpp Interpreter/main.cpp)
target_link_libraries(MiniLangCpp PRIVATE MiniLangCore)
install(PROGRAMS "$<TARGET_FILE:MiniLangCpp>" DESTINATION bin RENAME "mini${CMAKE_EXECUTABLE_SUFFIX}")

# Тесты

if(BUILD_TESTING)
  add_executable(UnitTests
          Interpreter/tests/LexerTests.cpp
          Interpreter/tests/ExpressionTreeTests.cpp
          Interpreter/tests/ParserTests.cpp
          Interpreter/tests/InterpreterTests.cpp
          SmartPointers/tests/SharedPtrTests.cpp
          SmartPointers/tests/UniquePtrTests.cpp
          SmartPointers/tests/SmartPointerSafetyTests.cpp
          DataStructures/tests/Array/ArrayTests.cpp
          DataStructures/tests/Trees/BinaryTreeTests.cpp
          SecondLab/tests/AlphabeticalIndexTests.cpp
          ThirdLab/tests/ShortestPathTests.cpp
          DataStructures/tests/Timer.h
      )

  find_package(Threads REQUIRED)

  target_link_libraries(UnitTests PRIVATE
          MiniLangCore
          SecondLabIndexes
          Threads::Threads
          SmartPointer
          BinaryTree
          ThirdLabPaths
          Array
          GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(UnitTests)

  add_executable(TreeSafetyTests DataStructures/tests/Trees/TreeSafetyTests.cpp)
  target_link_libraries(TreeSafetyTests PRIVATE BinaryTree MiniLangCore GTest::gtest_main)
  gtest_discover_tests(TreeSafetyTests)

  add_executable(SharedPtrAllocationTests SmartPointers/tests/SharedPtrAllocationTests.cpp)
  target_link_libraries(SharedPtrAllocationTests PRIVATE SmartPointer GTest::gtest_main)
  gtest_discover_tests(SharedPtrAllocationTests)

  add_test(NAME MiniLangCliTests
    COMMAND ${CMAKE_COMMAND}
      "-DMINILANG_EXECUTABLE=$<TARGET_FILE:MiniLangCpp>"
      "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/minilang-cli-tests"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/Interpreter/tests/CliTests.cmake"
  )
  add_test(NAME SecondLabCliTests
    COMMAND ${CMAKE_COMMAND}
      "-DSECOND_LAB_EXECUTABLE=$<TARGET_FILE:DataStructures>"
      "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/second-lab-cli-tests"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/SecondLab/tests/CliTests.cmake"
  )
endif()

# Графический интерфейс собирается отдельно от консольной версии.
option(BUILD_GUI "Build Qt 6 Widgets application" OFF)
if(BUILD_GUI)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Concurrent)
  add_executable(TreesGui GUI/source/TreesGui.cpp)
  target_include_directories(TreesGui PRIVATE DataStructures/Trees)
  target_link_libraries(TreesGui PRIVATE BinaryTree Qt6::Widgets Qt6::Concurrent)

  add_library(DataStructuresGuiWidgets STATIC
          GUI/source/AlphabeticalIndexWidget.cpp
          GUI/source/PersonIndexWidget.cpp
          GUI/source/GraphWidget.cpp
          GUI/source/GraphScene.cpp
  )
  target_include_directories(DataStructuresGuiWidgets PUBLIC GUI/include)
  target_link_libraries(DataStructuresGuiWidgets
      PUBLIC ThirdLabPaths Qt6::Widgets Qt6::Concurrent
      PRIVATE SecondLabIndexes
  )
  add_executable(DataStructuresGui GUI/source/main.cpp)
  target_link_libraries(DataStructuresGui PRIVATE DataStructuresGuiWidgets)
  if(BUILD_TESTING)
    add_executable(GuiTests GUI/tests/GuiTests.cpp)
    target_link_libraries(GuiTests PRIVATE DataStructuresGuiWidgets GTest::gtest)
    add_test(NAME GuiTests COMMAND GuiTests)
    set_tests_properties(GuiTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
  endif()
endif()
