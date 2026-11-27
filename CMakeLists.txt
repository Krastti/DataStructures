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
        Labs/FirstLab/headers/BinaryTree.h
        Labs/FirstLab/headers/BinaryTree.tpp
        Labs/FirstLab/headers/RedBlackTree.h
        Labs/FirstLab/headers/RedBlackTree.tpp
        Labs/FirstLab/headers/Set.h
        Labs/FirstLab/headers/Set.tpp
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
        Labs/SmartPointers/include/UniquePtr.h
        Labs/SmartPointers/include/UniquePtr.tpp
        Labs/SmartPointers/include/SharedPtr.h
        Labs/SmartPointers/include/SharedPtr.tpp
)

add_library(Optional INTERFACE
        DataStructures/Optional/Optional.h
        DataStructures/Optional/Optional.tpp
)

add_library(Graph INTERFACE
        DataStructures/Graph/Graph.h
        DataStructures/Graph/AGraph.h
        DataStructures/Graph/AGraph.tpp
)

add_library(ThirdLabPaths INTERFACE
        Labs/ThirdLab/headers/algoritms/ShortestPath.h
        Labs/ThirdLab/headers/algoritms/ShortestPath.tpp
)

add_library(ThirdLabStateMachine INTERFACE
        Labs/ThirdLab/headers/StateMachine/StateMachine.h
        Labs/ThirdLab/headers/StateMachine/StateMachine.tpp
)

foreach(module IN ITEMS Array Sequence Stack Optional Graph)
  target_include_directories(${module} INTERFACE DataStructures)
endforeach()
target_include_directories(SmartPointer INTERFACE Labs/SmartPointers/include)

target_include_directories(BinaryTree INTERFACE Labs/FirstLab/headers)
target_link_libraries(BinaryTree INTERFACE Array Stack)
target_link_libraries(Graph INTERFACE Array BinaryTree)
target_include_directories(ThirdLabPaths INTERFACE Labs/ThirdLab/headers/algoritms)
target_link_libraries(ThirdLabPaths INTERFACE Graph Optional)
target_include_directories(ThirdLabStateMachine INTERFACE Labs/ThirdLab/headers)
target_link_libraries(ThirdLabStateMachine INTERFACE Array Graph)

add_library(SecondLabIndexes STATIC
        Labs/SecondLab/source/AlphabeticalIndex.cpp
        Labs/SecondLab/source/PersonIndex.cpp
)
target_include_directories(SecondLabIndexes PUBLIC Labs/SecondLab/include)
target_link_libraries(SecondLabIndexes PUBLIC Array BinaryTree SmartPointer)

add_executable(DataStructures main.cpp)
target_link_libraries(DataStructures PRIVATE SecondLabIndexes ThirdLabPaths Array)

# Общая реализация для CLI, GUI и тестов интерпретатора.
add_library(MiniLangCore STATIC
        Labs/FirstLab/headers/BinaryTreeBase.h
        Labs/FirstLab/headers/BinaryTreeBase.tpp
        Interpreter/sources/lexer/lexer.cpp
        Interpreter/sources/parser/parser.cpp
        Interpreter/headers/loader/loadState.h
        Interpreter/headers/loader/programLoader.h
        Interpreter/sources/loader/programLoader.cpp
        Interpreter/enums/tokenType.h
        Interpreter/headers/ast/expressionTree.h
        Interpreter/sources/ast/expressionTree.cpp
        Interpreter/sources/ast/ast.cpp
        Interpreter/sources/runtime/environment.cpp
        Interpreter/sources/runtime/interpreter.cpp
)
target_include_directories(MiniLangCore PUBLIC Interpreter/headers Interpreter/enums)
target_link_libraries(MiniLangCore PUBLIC Array BinaryTree Optional SmartPointer)
target_link_libraries(MiniLangCore PRIVATE ThirdLabStateMachine)

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

  add_executable(StackTests DataStructures/tests/Stack/StackTests.cpp)
  target_link_libraries(StackTests PRIVATE Stack GTest::gtest_main)
  gtest_discover_tests(StackTests PROPERTIES LABELS Stack)

  add_executable(TreesTests
          Labs/FirstLab/tests/BinaryTreeTests.cpp
          Labs/FirstLab/tests/RedBlackTree.cpp
          Labs/FirstLab/tests/SetTests.cpp
          DataStructures/tests/AllocationFailure.cpp
  )
  target_link_libraries(TreesTests PRIVATE BinaryTree GTest::gtest_main)
  gtest_discover_tests(TreesTests PROPERTIES LABELS Trees)

  add_executable(InterpreterTests
          Interpreter/tests/LexerTests.cpp
          Interpreter/tests/ExpressionTreeTests.cpp
          Interpreter/tests/ExpressionTreeAllocationTests.cpp
          Interpreter/tests/ParserTests.cpp
          Interpreter/tests/ProgramLoaderTests.cpp
          Interpreter/tests/InterpreterTests.cpp
          DataStructures/tests/AllocationFailure.cpp
  )
  target_link_libraries(InterpreterTests PRIVATE MiniLangCore GTest::gtest_main)
  add_dependencies(InterpreterTests MiniLangCpp)
  gtest_discover_tests(InterpreterTests PROPERTIES LABELS Interpreter)

  add_executable(SmartPointersTests
          Labs/SmartPointers/tests/SharedPtrTests.cpp
          Labs/SmartPointers/tests/UniquePtrTests.cpp
          Labs/SmartPointers/tests/SmartPointerSafetyTests.cpp
          Labs/SmartPointers/tests/SharedPtrAllocationTests.cpp
  )
  target_link_libraries(SmartPointersTests PRIVATE SmartPointer Threads::Threads GTest::gtest_main)
  gtest_discover_tests(SmartPointersTests PROPERTIES LABELS SmartPointers)

  add_executable(SecondLabTests Labs/SecondLab/tests/AlphabeticalIndexTests.cpp)
  target_link_libraries(SecondLabTests PRIVATE SecondLabIndexes GTest::gtest_main)
  add_dependencies(SecondLabTests DataStructures)
  gtest_discover_tests(SecondLabTests PROPERTIES LABELS SecondLab)

  add_executable(ThirdLabTests
          Labs/ThirdLab/tests/StateMachineTests.cpp
  )
  target_link_libraries(ThirdLabTests PRIVATE ThirdLabPaths ThirdLabStateMachine GTest::gtest_main)
  gtest_discover_tests(ThirdLabTests PROPERTIES LABELS ThirdLab)

  add_custom_target(UnitTests DEPENDS
          ArrayTests StackTests TreesTests InterpreterTests SmartPointersTests SecondLabTests ThirdLabTests
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
      -P "${CMAKE_CURRENT_SOURCE_DIR}/Labs/SecondLab/tests/CliTests.cmake"
  )
  set_tests_properties(SecondLabCliTests PROPERTIES LABELS SecondLab)
endif()

# Графический интерфейс собирается отдельно от консольной версии.
option(BUILD_GUI "Build Qt 6 Widgets application" OFF)
if(BUILD_GUI)
  find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Concurrent)
  add_executable(TreesGui GUI/source/TreesGui.cpp)
  target_link_libraries(TreesGui PRIVATE BinaryTree Qt6::Widgets Qt6::Concurrent)

  add_executable(MiniLangGui GUI/source/MiniLangGui.cpp)
  target_link_libraries(MiniLangGui PRIVATE MiniLangCore Qt6::Widgets)

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
    add_executable(MiniLangGuiTests GUI/tests/MiniLangGuiTests.cpp)
    target_link_libraries(MiniLangGuiTests PRIVATE MiniLangCore Qt6::Widgets GTest::gtest)
    add_test(NAME MiniLangGuiTests COMMAND MiniLangGuiTests)
    add_dependencies(UnitTests MiniLangGuiTests)
    set_tests_properties(MiniLangGuiTests PROPERTIES LABELS "GUI;Interpreter"
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    add_executable(GuiTests GUI/tests/GuiTests.cpp)
    target_link_libraries(GuiTests PRIVATE DataStructuresGuiWidgets GTest::gtest)
    add_test(NAME GuiTests COMMAND GuiTests)
    add_dependencies(UnitTests GuiTests)
    set_tests_properties(GuiTests PROPERTIES LABELS GUI ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
  endif()
endif()
