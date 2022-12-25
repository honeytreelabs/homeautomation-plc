add_library(runtime STATIC
  gv_factory.cpp
  i2c_factory.cpp
  io_factory.cpp
  modbus_factory.cpp
  mqtt_io_factory.cpp
  program_factory.cpp
  runtime_factory.cpp
  scheduler_factory.cpp)

target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_features(runtime PUBLIC cxx_std_20)

target_link_libraries(runtime PUBLIC
  common
  components::automation
  components::lua
  scheduler)

create_memchecked_test(gv_factory runtime TRUE)
create_memchecked_test(io_factory runtime TRUE)
create_memchecked_test(mqtt_io_factory runtime TRUE)
create_memchecked_test(modbus_factory runtime TRUE)
create_memchecked_test(scheduler_factory runtime TRUE)
create_memchecked_test(runtime_factory runtime TRUE)
