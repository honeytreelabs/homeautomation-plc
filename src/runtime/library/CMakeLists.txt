add_library(library-automation STATIC
  blind_state.cpp
  blind.cpp
  library_registry_lua.cpp)
add_library(library::automation ALIAS library-automation)
target_include_directories(library-automation PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(library-automation PUBLIC
  common
  spdlog::spdlog
  sol2::sol2
  lua)

create_memchecked_test(blind_state library-automation TRUE)
create_memchecked_test(blind library-automation TRUE)
create_memchecked_test(trigger library-automation TRUE)
create_memchecked_test(library_registry_lua library-automation TRUE)
