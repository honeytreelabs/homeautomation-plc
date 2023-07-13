find_package(libmodbus CONFIG REQUIRED)
find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
find_package(lua REQUIRED)
find_package(unofficial-libuuid CONFIG REQUIRED)

add_library(system STATIC
  modbus.cpp
  mqtt.cpp
  mqtt_c.cpp
  signal.cpp)
target_include_directories(system PUBLIC ${CMAKE_CURRENT_LIST_DIR})
# https://stackoverflow.com/questions/52135983/cmake-target-link-libraries-include-as-system-to-suppress-compiler-warnings
target_link_libraries(system PUBLIC
  modbus
  eclipse-paho-mqtt-c::paho-mqtt3a-static
  eclipse-paho-mqtt-c::paho-mqtt3c-static
  eclipse-paho-mqtt-c::paho-mqtt3as-static
  eclipse-paho-mqtt-c::paho-mqtt3cs-static
  unofficial::UUID::uuid
  spdlog::spdlog)

if(MSVC)
  target_compile_options(system PRIVATE /W4 /WX)
else()
  target_compile_options(system PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_features(system PUBLIC cxx_std_20)

create_memchecked_test(i2c_dev system TRUE)
# the following is a workaround and should work on the long run
# without any memory leaks
create_memchecked_test(mqtt system TRUE)
create_memchecked_test(messages_buffer system TRUE)
