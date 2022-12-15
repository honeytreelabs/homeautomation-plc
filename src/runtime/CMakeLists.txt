add_library(runtime STATIC
  gv_factory.cpp
  mqtt_factory.cpp
  io_factory.cpp
  scheduler_factory.cpp
  runtime_factory.cpp)

target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(runtime PRIVATE ${CMAKE_CURRENT_LIST_DIR}/private)

target_compile_features(runtime PUBLIC cxx_std_20)

target_link_libraries(runtime PUBLIC
  components-mqtt
  common
  scheduler
  system)

if (NOT BUILD_RUN_TESTS)
  return ()
endif()

create_memchecked_test(gv_factory runtime TRUE)
create_memchecked_test(scheduler_factory runtime TRUE)
# additional include directories needed for this whitebox test
target_include_directories(scheduler_factory_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/private)
create_memchecked_test(runtime_factory runtime TRUE)
