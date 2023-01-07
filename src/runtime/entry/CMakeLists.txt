add_library(entry STATIC main.cpp)
target_link_libraries(entry PUBLIC
  factory
  ${CONAN_LIBS}
)

add_library(null_program_factory STATIC null_program_factory.cpp)
target_link_libraries(null_program_factory PUBLIC
  runtime)
