cmake_minimum_required(VERSION 3.22.1)

project("loader")

add_library(loader SHARED loader.cpp)

find_library(log-lib log)

target_link_libraries(loader ${log-lib} dl)
target_compile_features(loader PRIVATE cxx_std_20)
