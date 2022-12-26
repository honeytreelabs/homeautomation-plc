add_library(components-automation STATIC blind.cpp)
add_library(components::automation ALIAS components-automation)
target_include_directories(components-automation PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(components-automation PUBLIC
  runtime
  spdlog::spdlog)

add_library(components-lua STATIC lua_block.cpp)
add_library(components::lua ALIAS components-lua)
target_include_directories(components-lua PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(components-lua PUBLIC
  runtime
  sol2::sol2
  lua)

create_memchecked_test(blind_state components-automation FALSE)
create_memchecked_test(blind components-automation FALSE)
create_memchecked_test(trigger components-automation FALSE)
create_memchecked_test(lua_block components-lua TRUE)
