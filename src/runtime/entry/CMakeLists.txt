add_library(entry STATIC main.cpp)
target_link_libraries(entry PUBLIC
  yaml-cpp::yaml-cpp
  runtime
  system
  ${CONAN_LIBS}
)
