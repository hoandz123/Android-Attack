cmake_minimum_required(VERSION 3.22.1)

project("attack")

find_package(native-core REQUIRED CONFIG)

add_library(attack SHARED native-lib.cpp)

find_library(log-lib log)
find_library(z-lib z)

# Vendored curl stack (Prefab only exports headers for native-core::curl)
set(_curl ${NATIVE_CORE_CPP}/curl/${ANDROID_ABI})

target_link_libraries(
    attack
    ${log-lib}
    native-core::kitty
    native-core::dobby
    native-core::imgui
    native-core::curl
    ${_curl}/libcurl.a
    ${_curl}/libngtcp2_crypto.a
    ${_curl}/libngtcp2.a
    ${_curl}/libnghttp3.a
    ${_curl}/libnghttp2.a
    ${_curl}/libssl.a
    ${_curl}/libcrypto.a
    ${z-lib}
)

target_compile_features(attack PRIVATE cxx_std_20)
target_compile_definitions(attack PRIVATE CURL_STATICLIB)
