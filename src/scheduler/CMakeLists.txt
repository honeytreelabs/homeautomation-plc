add_library(scheduler INTERFACE)
target_include_directories(scheduler INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(scheduler INTERFACE
  common
  spdlog::spdlog
  sol2::sol2
  lua
)
target_compile_features(scheduler INTERFACE cxx_std_20)
