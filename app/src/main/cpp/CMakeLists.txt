cmake_minimum_required(VERSION 3.22.1)
project("attack")

find_package(native-core REQUIRED CONFIG)
find_package(mod-ui REQUIRED CONFIG)

add_library(attack SHARED native-lib.cpp app_menu.cpp)

find_library(log-lib log)
find_library(z-lib z)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

# Repo root from app/src/main/cpp
set(_repo "${CMAKE_SOURCE_DIR}/../../../..")
set(_abi "android.${ANDROID_ABI}")
set(_dobby "${NATIVE_CORE_CPP}/dobby/${ANDROID_ABI}/libdobby.a")
set(_curl_all "${NATIVE_CORE_CPP}/curl/${ANDROID_ABI}/libcurl_all.a")
set(_modui "${_repo}/mod-ui/build/intermediates/prefab_package/release/prefab/modules/modui/libs/${_abi}/libmodui.a")
set(_imgui "${_repo}/native-core/build/intermediates/prefab_package/release/prefab/modules/imgui/libs/${_abi}/libimgui.a")

target_include_directories(attack PRIVATE
    ${NATIVE_CORE_CPP}
    ${NATIVE_CORE_CPP}/imgui
    ${NATIVE_CORE_CPP}/imgui/backends)

# Prefab chỉ export header; .a link tay giống dobby/curl.
target_link_libraries(attack
    ${log-lib}
    mod-ui::modui
    ${_modui}
    ${_imgui}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
    native-core::kitty
    native-core::dobby
    native-core::curl
    ${_dobby}
    ${_curl_all}
    ${z-lib}
)

target_compile_features(attack PRIVATE cxx_std_20)
target_compile_definitions(attack PRIVATE CURL_STATICLIB)
