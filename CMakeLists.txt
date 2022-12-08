cmake_policy(SET CMP0048 NEW)
project(homeautomation-cpp-reference)
cmake_minimum_required(VERSION 3.20)

# we want to build a statically linked binary only
set(BUILD_SHARED_LIBS FALSE)

enable_testing()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(deps)
add_subdirectory(src)
