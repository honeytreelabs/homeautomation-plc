add_library(entry STATIC main.cpp)
target_link_libraries(entry PUBLIC
  factory
)

add_executable(generic null_program_factory.cpp)
target_link_libraries(generic PRIVATE entry)
if (RASPBERRY)
  target_link_libraries(generic PUBLIC -static)
endif ()
