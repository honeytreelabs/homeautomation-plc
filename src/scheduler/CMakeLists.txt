add_library(scheduler STATIC scheduler.cpp)
target_include_directories(scheduler PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(scheduler PUBLIC
  common
  system
  spdlog::spdlog
)
target_compile_features(scheduler PRIVATE cxx_std_20)
