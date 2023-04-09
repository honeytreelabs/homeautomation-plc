add_library(test_util STATIC
  subprocess.cpp
)

target_include_directories(test_util PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(test_util PUBLIC cxx_std_20)
