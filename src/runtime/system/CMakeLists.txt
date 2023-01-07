add_library(system STATIC
  modbus.cpp
  mqtt.cpp
  signal.cpp)
target_include_directories(system PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(system PUBLIC
  ${CONAN_LIBS}
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
create_memchecked_test(mqtt system FALSE)
create_memchecked_test(messages_buffer system TRUE)
