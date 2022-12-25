add_library(common INTERFACE)

target_include_directories(common INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_compile_features(common INTERFACE cxx_std_20)
