add_library(scheduler STATIC
  mqtt_impl.cpp)
target_include_directories(scheduler PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(scheduler PUBLIC
  common
  system
  spdlog::spdlog
)
target_compile_features(scheduler PUBLIC cxx_std_20)
