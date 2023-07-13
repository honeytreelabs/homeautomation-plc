find_package(Lua REQUIRED)

add_library(library-automation STATIC
  blind_state.cpp
  blind.cpp
  library_registry_lua.cpp
  time.cpp)
add_library(library::automation ALIAS library-automation)
target_include_directories(library-automation PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  "${LUA_INCLUDE_DIR}")
target_link_libraries(library-automation PUBLIC
  common
  spdlog::spdlog
  sol2::sol2
  "${LUA_LIBRARIES}"
  "${CMAKE_DL_LIBS}")

add_executable(library_lua_main library_lua_main.cpp)
target_include_directories(library_lua_main PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(library_lua_main PRIVATE SOL_PRINT_ERRORS=0)
target_link_libraries(library_lua_main PRIVATE library-automation)

add_test(NAME library_lua_main_test
  COMMAND $<TARGET_FILE:library_lua_main> ./test/test_library.lua
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

create_memchecked_test(blind library-automation TRUE)
create_memchecked_test(trigger library-automation TRUE)
create_memchecked_test(library_registry_lua library-automation TRUE)
create_memchecked_test(library_time library-automation TRUE)
