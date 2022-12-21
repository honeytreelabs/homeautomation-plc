add_subdirectory(test)

add_subdirectory(common)
add_subdirectory(system)
add_subdirectory(scheduler)
add_subdirectory(components)
add_subdirectory(runtime)
add_subdirectory(entry)

function(create_logic name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE
    components::automation
    entry
    runtime
    scheduler
    system
    components::lua)
  if (RASPBERRY)
    target_link_libraries(${name} PUBLIC -static)
  endif()
  if(MSVC)
    target_compile_options(${name} PRIVATE /W4 /WX)
  else()
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    # in the furute, -Werror shall be enabled too with few exceptions to that rule
  endif()
  target_compile_features(${name} PRIVATE cxx_std_20)

  set(binary ${name})
  configure_file(${CMAKE_SOURCE_DIR}/deploy/homeautomation.init.in homeautomation.${name})
  unset(binary)
endfunction()

create_logic(roof roof.cpp)
create_logic(ground ground.cpp)
create_logic(basement basement.cpp)
