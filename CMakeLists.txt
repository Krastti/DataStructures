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
        FirstLab/headers/BinaryTree.h
        FirstLab/headers/BinaryTree.tpp
        FirstLab/headers/RedBlackTree.h
        FirstLab/headers/RedBlackTree.tpp
        FirstLab/headers/Set.h
        FirstLab/headers/Set.tpp
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

foreach(module IN ITEMS Array Sequence Stack Optional Graph)
  target_include_directories(${module} INTERFACE DataStructures)
endforeach()
target_include_directories(SmartPointer INTERFACE SmartPointers/include)

target_include_directories(BinaryTree INTERFACE FirstLab/headers)
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
        FirstLab/headers/BinaryTreeBase.h
        FirstLab/headers/BinaryTreeBase.tpp
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
  find_package(Threads REQUIRED)
  include(GoogleTest)

  add_executable(ArrayTests DataStructures/tests/Array/ArrayTests.cpp)
  target_link_libraries(ArrayTests PRIVATE Array GTest::gtest_main)
  gtest_discover_tests(ArrayTests PROPERTIES LABELS Array)

  add_executable(TreesTests
          FirstLab/tests/BinaryTreeTests.cpp
          FirstLab/tests/TreeSafetyTests.cpp
          DataStructures/tests/AllocationFailure.cpp
  )
  target_link_libraries(TreesTests PRIVATE BinaryTree GTest::gtest_main)
  gtest_discover_tests(TreesTests PROPERTIES LABELS Trees)

  add_executable(InterpreterTests
          Interpreter/tests/LexerTests.cpp
          Interpreter/tests/ExpressionTreeTests.cpp
          Interpreter/tests/ExpressionTreeAllocationTests.cpp
          Interpreter/tests/ParserTests.cpp
          Interpreter/tests/InterpreterTests.cpp
          DataStructures/tests/AllocationFailure.cpp
  )
  target_link_libraries(InterpreterTests PRIVATE MiniLangCore GTest::gtest_main)
  add_dependencies(InterpreterTests MiniLangCpp)
  gtest_discover_tests(InterpreterTests PROPERTIES LABELS Interpreter)

  add_executable(SmartPointersTests
          SmartPointers/tests/SharedPtrTests.cpp
          SmartPointers/tests/UniquePtrTests.cpp
          SmartPointers/tests/SmartPointerSafetyTests.cpp
          SmartPointers/tests/SharedPtrAllocationTests.cpp
  )
  target_link_libraries(SmartPointersTests PRIVATE SmartPointer Threads::Threads GTest::gtest_main)
  gtest_discover_tests(SmartPointersTests PROPERTIES LABELS SmartPointers)

  add_executable(SecondLabTests SecondLab/tests/AlphabeticalIndexTests.cpp)
  target_link_libraries(SecondLabTests PRIVATE SecondLabIndexes GTest::gtest_main)
  add_dependencies(SecondLabTests DataStructures)
  gtest_discover_tests(SecondLabTests PROPERTIES LABELS SecondLab)

  add_executable(ThirdLabTests ThirdLab/tests/ShortestPathTests.cpp)
  target_link_libraries(ThirdLabTests PRIVATE ThirdLabPaths GTest::gtest_main)
  gtest_discover_tests(ThirdLabTests PROPERTIES LABELS ThirdLab)

  add_custom_target(UnitTests DEPENDS
          ArrayTests TreesTests InterpreterTests SmartPointersTests SecondLabTests ThirdLabTests
  )

  add_test(NAME MiniLangCliTests
    COMMAND ${CMAKE_COMMAND}
      "-DMINILANG_EXECUTABLE=$<TARGET_FILE:MiniLangCpp>"
      "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/minilang-cli-tests"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/Interpreter/tests/CliTests.cmake"
  )
  set_tests_properties(MiniLangCliTests PROPERTIES LABELS Interpreter)

  add_test(NAME SecondLabCliTests
    COMMAND ${CMAKE_COMMAND}
      "-DSECOND_LAB_EXECUTABLE=$<TARGET_FILE:DataStructures>"
      "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/second-lab-cli-tests"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/SecondLab/tests/CliTests.cmake"
  )
  set_tests_properties(SecondLabCliTests PROPERTIES LABELS SecondLab)
endif()

# Графический интерфейс собирается отдельно от консольной версии.
option(BUILD_GUI "Build Qt 6 Widgets application" OFF)
if(BUILD_GUI)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Concurrent)
  add_executable(TreesGui GUI/source/TreesGui.cpp)
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
    add_dependencies(UnitTests GuiTests)
    set_tests_properties(GuiTests PROPERTIES LABELS GUI ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
  endif()
endif()
