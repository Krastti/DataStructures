function(check_cli name input)
  set(case_dir "${TEST_WORK_DIR}/${name}")
  file(MAKE_DIRECTORY "${case_dir}")
  file(WRITE "${case_dir}/input.txt" "${input}")
  execute_process(
    COMMAND "${SECOND_LAB_EXECUTABLE}"
    WORKING_DIRECTORY "${case_dir}"
    INPUT_FILE "${case_dir}/input.txt"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    TIMEOUT 10
  )
  if(NOT result STREQUAL "0" OR NOT error STREQUAL "")
    message(FATAL_ERROR "${name}: код ${result}, stderr: ${error}")
  endif()
  foreach(expected IN LISTS ARGN)
    string(FIND "${output}" "${expected}" position)
    if(position EQUAL -1)
      message(FATAL_ERROR "${name}: не найдено «${expected}» в выводе:\n${output}")
    endif()
  endforeach()
endfunction()

check_cli(word_found "1\n1\n4\nabcd abcd\n.\nabcd\n3\n0\n"
  "Бинарный поиск: 2 3"
  "Красно-чёрное дерево: 2 3"
  "Среднее время бинарного поиска, нс:"
  "Среднее время поиска в дереве, нс:"
  "Результаты совпадают: да"
)

check_cli(word_missing "1\n1\n4\nabcd\n.\nmissing\n3\n0\n"
  "Бинарный поиск: не найдено"
  "Красно-чёрное дерево: не найдено"
  "Результаты совпадают: да"
)

set(people "1\n2\n4\n1 Alan Turing 1912\n1 Ada Lovelace 1815\n1 Ada Lovelace 1815\n2 Grace Hopper 1906\nAda Lovelace 1815\n")
check_cli(people_compare "${people}1815 1912\n3\n0\n"
  "Результат индексированного поиска:\n1 Ada Lovelace 1815\n1 Ada Lovelace 1815\n2 Grace Hopper 1906\n1 Alan Turing 1912"
  "Результат полного просмотра:\n1 Alan Turing 1912\n1 Ada Lovelace 1815\n1 Ada Lovelace 1815\n2 Grace Hopper 1906"
  "Среднее время индексированного поиска, нс:"
  "Среднее время полного просмотра, нс:"
  "Результаты совпадают: да"
)

foreach(mode RANGE 1 3)
  check_cli("reversed_range_${mode}" "${people}1912 1815\n${mode}\n0\n"
    "Ошибка: Начало диапазона больше конца"
  )
endforeach()
