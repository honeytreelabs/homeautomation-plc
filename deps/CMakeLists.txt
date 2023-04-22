add_subdirectory(spdlog)
add_subdirectory(GSL)

set(SOL2_BUILD_LUA FALSE)
add_subdirectory(sol2)

add_subdirectory(yaml-cpp)

add_library(HFSM2 INTERFACE)
target_include_directories(HFSM2 INTERFACE "${CMAKE_CURRENT_LIST_DIR}/HFSM2/include")

if (BUILD_RUN_TESTS)
  add_subdirectory(doctest)

  include(CTest)
endif()
