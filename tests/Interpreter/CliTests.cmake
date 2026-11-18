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

check_cli(no_arguments 2 "" "Использование:")
check_cli(extra_arguments 2 "" "Использование:" "one.mini" "two.mini")
check_cli(success 0 "36.0\ntrue\n18.0\n" "" "${TEST_WORK_DIR}/success with spaces.mini")
check_cli(empty_file 0 "" "" "${TEST_WORK_DIR}/empty.mini")
check_cli(missing_file 1 "" "Ошибка чтения файла:" "${TEST_WORK_DIR}/missing.mini")
check_cli(directory 1 "" "Ошибка чтения файла:" "${TEST_WORK_DIR}")
check_cli(lexical_error 1 "" "Ошибка лексического анализа:" "${TEST_WORK_DIR}/lexical.mini")
check_cli(syntax_error 1 "" "Ошибка синтаксического анализа:" "${TEST_WORK_DIR}/syntax.mini")
check_cli(runtime_error 1 "1.0\n" "Ошибка выполнения: неизвестная переменная: missing"
  "${TEST_WORK_DIR}/runtime.mini")
