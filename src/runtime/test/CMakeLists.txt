IF (NOT BUILD_RUN_TESTS)
  return ()
ENDIF()

add_library(Catch2Static STATIC catch2wrapper.cpp)
target_link_libraries(Catch2Static PUBLIC Catch2::Catch2WithMain)
