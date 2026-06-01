cmake_minimum_required(VERSION 3.22.1)
project(native-core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(D ${CMAKE_CURRENT_SOURCE_DIR})
find_library(log-lib log)

# LGL headers — Logger, obfuscate, Macros, Utils (#include <Includes/...>)
add_library(lgl STATIC includes_link.cpp)
target_include_directories(lgl PUBLIC ${D})

# Dobby — LGL v5 prebuilt + shim for Prefab
set(_dobby ${D}/dobby/${ANDROID_ABI}/libdobby.a)
if(NOT EXISTS ${_dobby})
    message(FATAL_ERROR "Missing ${_dobby}")
endif()
add_library(dobby STATIC dobby/dobby_link.cpp)
target_include_directories(dobby PUBLIC ${D}/dobby)
target_link_libraries(dobby PUBLIC ${_dobby} lgl)

# KittyMemory — LGL v3.2
add_library(kitty STATIC
    kittymemory/KittyMemory.cpp kittymemory/KittyUtils.cpp
    kittymemory/MemoryBackup.cpp kittymemory/MemoryPatch.cpp)
target_include_directories(kitty PUBLIC ${D}/kittymemory)
target_compile_features(kitty PUBLIC cxx_std_17)
target_link_libraries(kitty PUBLIC lgl ${log-lib})

# ImGui — Android + GLES3 backend
add_library(imgui STATIC
    imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_android.cpp imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
target_compile_features(imgui PUBLIC cxx_std_20)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)
target_link_libraries(imgui PUBLIC lgl ${log-lib} ${android-lib} ${EGL-lib} ${GLESv3-lib})

# cURL — vendored vvb2060 prebuilts (no Maven)
set(_curl ${D}/curl/${ANDROID_ABI})
if(NOT EXISTS ${_curl}/libcurl.a)
    message(FATAL_ERROR "Missing ${_curl}/libcurl.a")
endif()
find_library(z-lib z)
macro(_curl_import name file)
    add_library(${name} STATIC IMPORTED)
    set_target_properties(${name} PROPERTIES IMPORTED_LOCATION ${_curl}/${file})
endmacro()
_curl_import(curl_prebuilt libcurl.a)
_curl_import(ngtcp2_crypto_prebuilt libngtcp2_crypto.a)
_curl_import(ngtcp2_prebuilt libngtcp2.a)
_curl_import(nghttp3_prebuilt libnghttp3.a)
_curl_import(nghttp2_prebuilt libnghttp2.a)
_curl_import(ssl_prebuilt libssl.a)
_curl_import(crypto_prebuilt libcrypto.a)
add_library(curl STATIC curl/curl_link.cpp)
target_include_directories(curl PUBLIC ${D}/curl/include)
target_compile_definitions(curl INTERFACE CURL_STATICLIB)
target_link_libraries(curl PUBLIC
    curl_prebuilt ngtcp2_crypto_prebuilt ngtcp2_prebuilt
    nghttp3_prebuilt nghttp2_prebuilt ssl_prebuilt crypto_prebuilt ${z-lib})
