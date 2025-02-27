add_library(json INTERFACE)
target_compile_features(json INTERFACE cxx_std_11)
target_include_directories(json INTERFACE include)