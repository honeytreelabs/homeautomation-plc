cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.20)
project(homeautomation-cpp-reference)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (ENABLE_COVERAGE)
  include(deps/cmake-modules/CodeCoverage.cmake)
  append_coverage_compiler_flags()
endif ()

# we want to build a statically linked binary only
set(BUILD_SHARED_LIBS FALSE)

enable_testing()

add_subdirectory(deps)
include(cmake/test_helper.cmake)
add_subdirectory(src)
add_subdirectory(examples)
