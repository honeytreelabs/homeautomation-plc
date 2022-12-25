add_library(entry STATIC main.cpp)
target_link_libraries(entry PUBLIC
  yaml-cpp::yaml-cpp
  factory
  system
  ${CONAN_LIBS}
)
