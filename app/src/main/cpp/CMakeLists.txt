cmake_minimum_required(VERSION 3.22.1)

project("attack")

find_package(native-core REQUIRED CONFIG)
find_package(curl REQUIRED CONFIG)

add_library(attack SHARED native-lib.cpp)

find_library(log-lib log)

target_link_libraries(
    attack
    ${log-lib}
    native-core::includes
    native-core::kitty
    native-core::dobby
    native-core::imgui
    curl::curl_static
)

target_compile_features(attack PRIVATE cxx_std_20)
target_compile_definitions(attack PRIVATE CURL_STATICLIB)
