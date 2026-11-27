if(NOT DEFINED MINILANG_EXECUTABLE OR NOT DEFINED TEST_WORK_DIR)
  message(FATAL_ERROR "Нужны MINILANG_EXECUTABLE и TEST_WORK_DIR")
endif()

file(MAKE_DIRECTORY "${TEST_WORK_DIR}")

function(check_cli name expected_code expected_output expected_error)
  execute_process(
    COMMAND "${MINILANG_EXECUTABLE}" ${ARGN}
    RESULT_VARIABLE actual_code
    OUTPUT_VARIABLE actual_output
    ERROR_VARIABLE actual_error
    TIMEOUT 10
  )
  if(NOT "${actual_code}" STREQUAL "${expected_code}")
    message(FATAL_ERROR "${name}: код ${actual_code} вместо ${expected_code}: ${actual_error}")
  endif()
  if(NOT "${actual_output}" STREQUAL "${expected_output}")
    message(FATAL_ERROR "${name}: неожиданный stdout: ${actual_output}")
  endif()
  if("${expected_error}" STREQUAL "")
    if(NOT "${actual_error}" STREQUAL "")
      message(FATAL_ERROR "${name}: неожиданный stderr: ${actual_error}")
    endif()
  else()
    string(FIND "${actual_error}" "${expected_error}" error_position)
    if(error_position EQUAL -1)
      message(FATAL_ERROR "${name}: неожиданная ошибка: ${actual_error}")
    endif()
  endif()
endfunction()

file(WRITE "${TEST_WORK_DIR}/success with spaces.mini"
  "remember x = 10\r\nremember y = (x + 2) * 3\r\nprint y\r\nprint y >= 30\r\nprint y / 2")
file(WRITE "${TEST_WORK_DIR}/empty.mini" "")
file(WRITE "${TEST_WORK_DIR}/lexical.mini" "print 1\n@")
file(WRITE "${TEST_WORK_DIR}/syntax.mini" "print 1\nprint (")
file(WRITE "${TEST_WORK_DIR}/runtime.mini" "print 1\nprint missing\nprint 2")
file(WRITE "${TEST_WORK_DIR}/control-flow.mini"
  "remember n = 0\n"
  "repeat while n < 3\n"
  "  when n == 1\n"
  "    print \"середина\"\n"
  "  otherwise\n"
  "    print n\n"
  "  end\n"
  "  remember n = n + 1\n"
  "end\n"
  "print n")
file(WRITE "${TEST_WORK_DIR}/unclosed-block.mini"
  "print 1\nwhen 1 < 2\n  repeat while 1 > 2\n  end")
file(WRITE "${TEST_WORK_DIR}/when-condition.mini"
  "when 1\nprint 2\nend")
file(WRITE "${TEST_WORK_DIR}/repeat-condition.mini"
  "print 1\nrepeat while \"text\"\nprint 2\nend\nprint 3")
file(WRITE "${TEST_WORK_DIR}/functions.mini"
  "function add(a, b) -> a + b end\n"
  "function twice(x) -> add(x, x) end\n"
  "print twice(3 + 2) * 2")
file(WRITE "${TEST_WORK_DIR}/factorial.mini"
  "function factorial(n) ->\n"
  "  remember result = 1\n"
  "  when n > 1\n"
  "    remember result = n * factorial(n - 1)\n"
  "  end\n"
  "  return result\n"
  "end\n"
  "print factorial(5)")

check_cli(no_arguments 2 "" "Использование:")
check_cli(extra_arguments 2 "" "Использование:" "one.mini" "two.mini")
check_cli(success 0 "36.0\ntrue\n18.0\n" "" "${TEST_WORK_DIR}/success with spaces.mini")
check_cli(empty_file 0 "" "" "${TEST_WORK_DIR}/empty.mini")
check_cli(missing_file 1 "" "Ошибка чтения файла:" "${TEST_WORK_DIR}/missing.mini")
check_cli(directory 1 "" "Ошибка чтения файла:" "${TEST_WORK_DIR}")
check_cli(lexical_error 1 "" "Ошибка лексического анализа в" "${TEST_WORK_DIR}/lexical.mini")
check_cli(syntax_error 1 "" "Ошибка синтаксического анализа в" "${TEST_WORK_DIR}/syntax.mini")
check_cli(runtime_error 1 "1.0\n" "Ошибка выполнения: неизвестная переменная: missing"
  "${TEST_WORK_DIR}/runtime.mini")
check_cli(control_flow 0 "0.0\nсередина\n2.0\n3.0\n" ""
  "${TEST_WORK_DIR}/control-flow.mini")
check_cli(unclosed_block 1 "" "Ожидался end для блока when"
  "${TEST_WORK_DIR}/unclosed-block.mini")
check_cli(when_condition_type 1 "" "Ошибка выполнения: условие when должно быть логическим"
  "${TEST_WORK_DIR}/when-condition.mini")
check_cli(repeat_condition_type 1 "1.0\n" "Ошибка выполнения: условие repeat while должно быть логическим"
  "${TEST_WORK_DIR}/repeat-condition.mini")
check_cli(functions 0 "20.0\n" "" "${TEST_WORK_DIR}/functions.mini")
check_cli(factorial 0 "120.0\n" "" "${TEST_WORK_DIR}/factorial.mini")
file(WRITE "${TEST_WORK_DIR}/updates.mini"
  "remember x = 1.5\nprint x++\nx += 2.25\nx -= 0.25\nprint --x")
check_cli(updates 0 "1.5\n3.5\n" "" "${TEST_WORK_DIR}/updates.mini")

file(MAKE_DIRECTORY "${TEST_WORK_DIR}/using/lib")
file(WRITE "${TEST_WORK_DIR}/using/main.mini"
  "using \"lib/math.mini\"\nprint square(5)")
file(WRITE "${TEST_WORK_DIR}/using/lib/math.mini"
  "function square(x) -> x * x end")
check_cli(using_source_directory 0 "25.0\n" "" "${TEST_WORK_DIR}/using/main.mini")

# Рабочий каталог отличается от каталога исходников во всех вызовах ниже.
file(MAKE_DIRECTORY "${TEST_WORK_DIR}/other-cwd")
function(check_using_cli name expected_code expected_output expected_error path)
  execute_process(
    COMMAND "${MINILANG_EXECUTABLE}" "${path}"
    WORKING_DIRECTORY "${TEST_WORK_DIR}/other-cwd"
    RESULT_VARIABLE code OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10
  )
  if(NOT "${code}" STREQUAL "${expected_code}" OR NOT "${output}" STREQUAL "${expected_output}")
    message(FATAL_ERROR "${name}: код ${code}, stdout ${output}, stderr ${error}")
  endif()
  if("${expected_error}" STREQUAL "")
    if(NOT "${error}" STREQUAL "")
      message(FATAL_ERROR "${name}: неожиданный stderr: ${error}")
    endif()
  else()
    string(FIND "${error}" "${expected_error}" position)
    if(position EQUAL -1)
      message(FATAL_ERROR "${name}: неожиданная ошибка: ${error}")
    endif()
    string(REGEX MATCHALL "Ошибка " prefixes "${error}")
    list(LENGTH prefixes prefix_count)
    if(NOT prefix_count EQUAL 1)
      message(FATAL_ERROR "${name}: этап ошибки выведен несколько раз: ${error}")
    endif()
  endif()
endfunction()

file(WRITE "${TEST_WORK_DIR}/using/lib/constants.mini" "function factor() -> 2 end")
file(WRITE "${TEST_WORK_DIR}/using/lib/math.mini"
  "using \"constants.mini\"\nfunction square(x) -> x * x * factor() end")
check_using_cli(nested_library 0 "50.0\n" "" "../using/main.mini")
file(WRITE "${TEST_WORK_DIR}/using/repeated.mini"
  "using \"lib/math.mini\"\nusing \"lib/./math.mini\"\nprint square(5)")
check_using_cli(repeated_library 0 "50.0\n" "" "../using/repeated.mini")
file(WRITE "${TEST_WORK_DIR}/using/duplicate.mini"
  "using \"lib/math.mini\"\nfunction square(x) -> x end\nprint square(5)")
check_using_cli(duplicate_function 1 "" "повторное объявление функции: square" "../using/duplicate.mini")
file(WRITE "${TEST_WORK_DIR}/using/missing.mini"
  "using \"missing-library.mini\"\nprint 123")
check_using_cli(missing_library 1 "" "missing-library.mini" "../using/missing.mini")
file(WRITE "${TEST_WORK_DIR}/using/lib/bad.mini" "function f() -> 1. end")
file(WRITE "${TEST_WORK_DIR}/using/bad.mini" "using \"lib/bad.mini\"\nprint 123")
check_using_cli(library_lexical_error 1 "" "Ошибка лексического анализа в" "../using/bad.mini")
file(WRITE "${TEST_WORK_DIR}/using/lib/bad.mini" "print 123")
check_using_cli(library_syntax_error 1 "" "Ошибка синтаксического анализа в" "../using/bad.mini")
file(WRITE "${TEST_WORK_DIR}/using/lib/a.mini" "using \"b.mini\"")
file(WRITE "${TEST_WORK_DIR}/using/lib/b.mini" "using \"a.mini\"")
file(WRITE "${TEST_WORK_DIR}/using/cycle.mini" "using \"lib/a.mini\"\nprint 123")
check_using_cli(library_cycle 1 "" "Циклическая зависимость" "../using/cycle.mini")
file(WRITE "${TEST_WORK_DIR}/using/lib/bad.mini" "function f() -> missing end")
file(WRITE "${TEST_WORK_DIR}/using/runtime.mini" "using \"lib/bad.mini\"\nprint 1\nprint f()")
check_using_cli(library_runtime_error 1 "1.0\n" "неизвестная переменная: missing" "../using/runtime.mini")

file(WRITE "${TEST_WORK_DIR}/negative.mini"
  "function identity(n) -> n end\nremember x = -0.5\nprint -5\nprint 0 - (x * 2)\nprint identity(-3)\nprint -0")
check_cli(negative_numbers 0 "-5.0\n1.0\n-3.0\n-0.0\n" "" "${TEST_WORK_DIR}/negative.mini")
file(WRITE "${TEST_WORK_DIR}/using/negative.mini" "using \"lib/math.mini\"\nprint square(-5)")
check_using_cli(negative_library_argument 0 "50.0\n" "" "../using/negative.mini")

file(WRITE "${TEST_WORK_DIR}/invalid-minus.mini" "print -(2 + 3)")
check_cli(minus_requires_number 1 "" "Ожидалось число после минуса" "${TEST_WORK_DIR}/invalid-minus.mini")
