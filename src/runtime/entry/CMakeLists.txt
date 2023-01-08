add_library(entry STATIC main.cpp)
target_link_libraries(entry PUBLIC
  factory
  ${CONAN_LIBS}
)

add_executable(generic null_program_factory.cpp)
target_link_libraries(generic PRIVATE
  entry)
