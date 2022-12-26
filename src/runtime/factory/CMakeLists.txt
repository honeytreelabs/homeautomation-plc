add_library(factory STATIC
  factory_helpers.cpp
  gv_factory.cpp
  i2c_factory.cpp
  io_factory.cpp
  modbus_factory.cpp
  mqtt_io_factory.cpp
  program_factory.cpp
  runtime_factory.cpp
  scheduler_factory.cpp)

target_include_directories(factory PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_features(factory PUBLIC cxx_std_20)

target_link_libraries(factory PUBLIC
  runtime
  yaml-cpp::yaml-cpp)

create_memchecked_test(gv_factory factory TRUE)
create_memchecked_test(io_factory factory TRUE)
create_memchecked_test(mqtt_io_factory factory TRUE)
create_memchecked_test(modbus_factory factory TRUE)
create_memchecked_test(program_factory factory TRUE)
create_memchecked_test(scheduler_factory factory TRUE)
create_memchecked_test(runtime_factory factory TRUE)
