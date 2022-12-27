add_library(runtime STATIC
  program_lua.cpp
  task_io_mqtt.cpp)
target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(runtime PUBLIC
  common
  system
  spdlog::spdlog
  sol2::sol2
  lua)
target_compile_features(runtime PUBLIC cxx_std_20)
