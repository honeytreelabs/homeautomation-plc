add_subdirectory(common)
add_subdirectory(system)
add_subdirectory(scheduler)
add_subdirectory(components)

function(create_logic name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE
    automation
    scheduler
    CLI11_SINGLE
    mqtt)
  if (RASPBERRY)
    target_link_libraries(${name} PUBLIC -static)
  endif()
  if(MSVC)
    target_compile_options(${name} PRIVATE /W4 /WX)
  else()
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  target_compile_features(${name} PRIVATE cxx_std_20)
endfunction()

create_logic(roof roof.cpp)
create_logic(ground ground.cpp)
