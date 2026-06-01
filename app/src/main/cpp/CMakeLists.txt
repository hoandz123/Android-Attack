cmake_minimum_required(VERSION 3.22.1)

project("attack")

find_package(native-core REQUIRED CONFIG)

add_library(attack SHARED native-lib.cpp)

find_library(log-lib log)
find_library(z-lib z)

set(_curl_all ${NATIVE_CORE_CPP}/curl/${ANDROID_ABI}/libcurl_all.a)

target_link_libraries(
    attack
    ${log-lib}
    native-core::kitty
    native-core::dobby
    native-core::imgui
    native-core::curl
    ${_curl_all}
    ${z-lib}
)

target_compile_features(attack PRIVATE cxx_std_20)
target_compile_definitions(attack PRIVATE CURL_STATICLIB)
