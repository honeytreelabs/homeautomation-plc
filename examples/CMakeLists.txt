function(create_init_script name)
  set(binary ${name})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/deploy/homeautomation.init.in homeautomation.${name})
  unset(binary)
endfunction()

function(create_logic name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE entry)
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
endfunction()

create_logic(ground ground.cpp)
create_init_script(ground)

create_logic(basement basement.cpp)
create_init_script(basement)

create_init_script(roof)
