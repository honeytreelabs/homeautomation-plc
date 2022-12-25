add_library(runtime STATIC
  mqtt_impl.cpp)
target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(runtime PUBLIC
  common
  system
  spdlog::spdlog
)
target_compile_features(runtime PUBLIC cxx_std_20)
