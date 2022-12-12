add_library(scheduler STATIC lua_program.cpp)
target_include_directories(scheduler PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(scheduler PUBLIC
  common
  spdlog::spdlog
  sol2::sol2
  lua
)
target_compile_features(scheduler PRIVATE cxx_std_20)
