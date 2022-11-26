add_library(components-automation STATIC
  blind.cpp)
add_library(components::automation ALIAS components-automation)

target_include_directories(components-automation PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(components-automation PUBLIC
  common
  spdlog::spdlog)

add_library(components-mqtt STATIC
  mqtt.cpp)
add_library(components::mqtt ALIAS components-mqtt)

target_include_directories(components-mqtt PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(components-mqtt PUBLIC ${CONAN_LIBS})

add_library(components-lua STATIC
  lua_block.cpp)
add_library(components::lua ALIAS components-lua)

target_include_directories(components-lua PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(components-lua PUBLIC
  common
  sol2::sol2
  lua
)

if (NOT BUILD_RUN_TESTS)
  return ()
endif()

function(create_components_test name lib memchecked)
  add_executable(${name}_test ${name}_test.cpp)
  target_link_libraries(${name}_test PRIVATE
  Catch2::Catch2WithMain
  ${lib})

  catch_discover_tests(${name}_test WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
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

create_components_test(blind_state components-automation FALSE)
create_components_test(blind components-automation FALSE)
create_components_test(trigger components-automation FALSE)
create_components_test(lua_block components-lua TRUE)
