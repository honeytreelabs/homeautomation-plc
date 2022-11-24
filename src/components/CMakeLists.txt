add_library(components-automation STATIC
  blind.cpp)
add_library(components::automation ALIAS components-automation)

target_include_directories(components-automation PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(components-automation PUBLIC
  common
  spdlog::spdlog)

add_library(components-mqtt STATIC
  mqtt.cpp)
add_library(components::mqtt ALIAS components-mqtt)

target_include_directories(components-mqtt PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(components-mqtt PUBLIC ${CONAN_LIBS})

add_library(components-lua STATIC
  lua.cpp)
add_library(components::lua ALIAS components-lua)

target_link_libraries(components-lua PUBLIC sol2::sol2)

if (NOT BUILD_RUN_TESTS)
  return ()
endif()

add_executable(blind_state_test blind_state_test.cpp)
target_link_libraries(blind_state_test PRIVATE
  Catch2::Catch2WithMain
  components-automation)

catch_discover_tests(blind_state_test)

add_executable(blind_test blind_test.cpp)
target_link_libraries(blind_test PRIVATE
  Catch2::Catch2WithMain
  components-automation)

catch_discover_tests(blind_test)

add_executable(trigger_test trigger_test.cpp)
target_link_libraries(trigger_test PRIVATE
  Catch2::Catch2WithMain
  components-automation)

catch_discover_tests(trigger_test)
