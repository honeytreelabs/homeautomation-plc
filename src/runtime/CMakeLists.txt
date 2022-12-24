add_library(runtime STATIC
  gv_factory.cpp
  i2c_factory.cpp
  io_factory.cpp
  modbus_factory.cpp
  mqtt_impl.cpp
  mqtt_io_factory.cpp
  program_factory.cpp
  runtime_factory.cpp
  scheduler_factory.cpp)

target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(runtime PRIVATE ${CMAKE_CURRENT_LIST_DIR}/private)

target_compile_features(runtime PUBLIC cxx_std_20)

target_link_libraries(runtime PUBLIC
  common
  scheduler
  system)

function(create_memchecked_whitebox_test name lib)
  create_memchecked_test(${name} ${lib} TRUE)
  if (TARGET ${name}_test)
    # additional include directories needed for this whitebox test
    target_include_directories(${name}_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/private)
  endif ()
endfunction()
create_memchecked_test(gv_factory runtime TRUE)
create_memchecked_whitebox_test(io_factory runtime TRUE)
create_memchecked_whitebox_test(mqtt_io_factory runtime TRUE)
create_memchecked_whitebox_test(modbus_factory runtime TRUE)
create_memchecked_whitebox_test(scheduler_factory runtime TRUE)
create_memchecked_test(runtime_factory runtime TRUE)
