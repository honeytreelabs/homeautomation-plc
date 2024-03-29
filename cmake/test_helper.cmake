if (BUILD_RUN_TESTS)
  function(create_memchecked_test name lib memchecked)
    add_executable(${name}_test ${name}_test.cpp)
    target_link_libraries(${name}_test PUBLIC
      doctest::doctest
      test_util
      ${lib})

    add_test(NAME ${name}_test
      COMMAND $<TARGET_FILE:${name}_test>
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    if (${memchecked})
      add_test(NAME ${name}_memchecked_test
      COMMAND valgrind
        --error-exitcode=1
        --tool=memcheck
        --leak-check=full
        --errors-for-leak-kinds=definite
        --show-leak-kinds=definite $<TARGET_FILE:${name}_test>
      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    endif()
  endfunction()
else ()
  # empty functions if not testing is enabled
  function(create_memchecked_test name lib memchecked)
  endfunction()
endif ()
