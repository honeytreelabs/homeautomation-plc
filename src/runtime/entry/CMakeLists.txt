add_library(entry STATIC main.cpp)
target_include_directories(entry PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(entry PUBLIC
  yaml-cpp::yaml-cpp
  runtime
  system
  ${CONAN_LIBS}
)
