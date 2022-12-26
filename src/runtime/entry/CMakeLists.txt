add_library(entry STATIC main.cpp)
target_link_libraries(entry PUBLIC
  factory
  ${CONAN_LIBS}
)
