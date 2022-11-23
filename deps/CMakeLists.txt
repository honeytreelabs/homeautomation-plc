add_subdirectory(spdlog)
set(CLI11_SINGLE_FILE ON)
add_subdirectory(CLI11)
add_subdirectory(GSL)

set(SOL2_BUILD_LUA FALSE)
add_subdirectory(sol2)

if (BUILD_RUN_TESTS)
  add_subdirectory(Catch2)

  include(CTest)
  include(Catch)
endif()
