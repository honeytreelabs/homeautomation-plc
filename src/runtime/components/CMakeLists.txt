add_library(components-automation STATIC blind.cpp)
add_library(components::automation ALIAS components-automation)
target_include_directories(components-automation PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(components-automation PUBLIC
  runtime
  spdlog::spdlog)

create_memchecked_test(blind_state components-automation FALSE)
create_memchecked_test(blind components-automation FALSE)
create_memchecked_test(trigger components-automation FALSE)
