find_package(Lua REQUIRED)

add_library(runtime STATIC
  program_lua.cpp
  task_io_mqtt.cpp)
target_include_directories(runtime PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  "${LUA_INCLUDE_DIR}")
target_link_libraries(runtime PUBLIC
  common
  system
  library::automation
  spdlog::spdlog
  sol2::sol2
  "${LUA_LIBRARIES}"
  "${CMAKE_DL_LIBS}")
target_compile_features(runtime PUBLIC cxx_std_20)

create_memchecked_test(program_lua runtime TRUE)
