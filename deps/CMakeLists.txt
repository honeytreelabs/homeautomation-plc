add_subdirectory(spdlog)
add_subdirectory(GSL)

set(SOL2_BUILD_LUA FALSE)
add_subdirectory(sol2)

add_subdirectory(yaml-cpp)

if (BUILD_RUN_TESTS)
  add_subdirectory(doctest)

  include(CTest)
endif()
