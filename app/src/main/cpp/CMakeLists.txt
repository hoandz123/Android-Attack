cmake_minimum_required(VERSION 3.22.1)

project("attack")

add_library(
    attack
    SHARED
    native-lib.cpp
)

find_library(log-lib log)

target_link_libraries(
    attack
    ${log-lib}
)

target_compile_features(attack PRIVATE cxx_std_20)
