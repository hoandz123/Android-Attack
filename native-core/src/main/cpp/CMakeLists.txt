cmake_minimum_required(VERSION 3.22.1)
project(native-core)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(D ${CMAKE_CURRENT_SOURCE_DIR})
find_library(log-lib log)

# =============================================================================
# LGL headers — Logger, obfuscate, Macros, Utils, oxorany (#include <Includes/...>)
# =============================================================================
add_library(lgl STATIC IncludesLink.cpp Includes/oxorany.cpp)
target_include_directories(lgl PUBLIC ${D})

# =============================================================================
# Dobby — LGL v5 prebuilt + shim for Prefab
# =============================================================================
set(_dobby ${D}/dobby/${ANDROID_ABI}/libdobby.a)
if(NOT EXISTS ${_dobby})
    message(FATAL_ERROR "Missing ${_dobby}")
endif()
add_library(dobby STATIC dobby/DobbyLink.cpp)
target_include_directories(dobby PUBLIC ${D}/dobby)
target_link_libraries(dobby PUBLIC ${_dobby} lgl)

# =============================================================================
# KittyMemory — LGL v3.2
# =============================================================================
add_library(kitty STATIC
    kittymemory/KittyMemory.cpp kittymemory/KittyUtils.cpp
    kittymemory/MemoryBackup.cpp kittymemory/MemoryPatch.cpp)
target_include_directories(kitty PUBLIC ${D}/kittymemory)
target_compile_features(kitty PUBLIC cxx_std_17)
target_link_libraries(kitty PUBLIC lgl ${log-lib})

# =============================================================================
# JNIHelper — JavaVM + JNIEnv thống nhất (không Detach thread đã attach)
# =============================================================================
add_library(jnihelper STATIC JNIHelper/JNIHelper.cpp)
target_include_directories(jnihelper PUBLIC ${D})
target_compile_features(jnihelper PUBLIC cxx_std_20)
target_link_libraries(jnihelper PUBLIC ${log-lib})

# =============================================================================
# DexLoader — in-memory DEX into PathClassLoader (makeInMemoryDexElements)
# =============================================================================
add_library(dexloader STATIC
    DexLoader/DexLoader.cpp
    DexLoader/JniReflect.cpp)
target_include_directories(dexloader PUBLIC ${D})
target_compile_features(dexloader PUBLIC cxx_std_20)
target_link_libraries(dexloader PUBLIC jnihelper lgl ${log-lib})

# =============================================================================
# ActivityTracker — JVM + running Activity contexts
# =============================================================================
add_library(activitytracker STATIC ActivityTracker/ActivityTracker.cpp)
target_include_directories(activitytracker PUBLIC ${D})
target_compile_features(activitytracker PUBLIC cxx_std_20)
target_link_libraries(activitytracker PUBLIC jnihelper lgl ${log-lib})

# =============================================================================
# ImGui — Android + GLES3 backend
# =============================================================================
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

# =============================================================================
# cURL — vendored stack merged into libcurl_all.a per ABI
# =============================================================================
set(_curl_all ${D}/curl/${ANDROID_ABI}/libcurl_all.a)
if(NOT EXISTS ${_curl_all})
    message(FATAL_ERROR "Missing ${_curl_all}")
endif()
find_library(z-lib z)
add_library(curl STATIC curl/CurlLink.cpp)
target_include_directories(curl PUBLIC ${D}/curl/include)
target_compile_definitions(curl INTERFACE CURL_STATICLIB)
target_link_libraries(curl PUBLIC ${_curl_all} ${z-lib})

# =============================================================================
# HttpClient — GET / POST / Download (libcurl)
# =============================================================================
add_library(httpclient STATIC HttpClient/HttpClient.cpp)
target_include_directories(httpclient PUBLIC ${D}/HttpClient ${D}/curl/include)
target_compile_features(httpclient PUBLIC cxx_std_20)
target_link_libraries(httpclient PUBLIC curl filemanager lgl ${log-lib})

# =============================================================================
# FileManager — đọc/ghi, thư mục, copy, xóa (std::filesystem)
# =============================================================================
add_library(filemanager STATIC FileManager/FileManager.cpp)
target_include_directories(filemanager PUBLIC ${D}/FileManager ${D})
target_compile_features(filemanager PUBLIC cxx_std_20)
target_link_libraries(filemanager PUBLIC lgl)

# =============================================================================
# xdl — dlopen/dlsym/dladdr bypass linker namespace (C sources)
# =============================================================================
add_library(xdl STATIC
    xdl/xdl.c xdl/xdl_iterate.c xdl/xdl_linker.c xdl/xdl_lzma.c xdl/xdl_util.c)
target_include_directories(xdl PUBLIC ${D}/xdl)
target_link_libraries(xdl PUBLIC ${log-lib})

# =============================================================================
# xhook — PLT/GOT hook cho ELF (C sources)
# =============================================================================
add_library(xhook STATIC
    xhook/xhook.c xhook/xh_core.c xhook/xh_elf.c xhook/xh_jni.c
    xhook/xh_log.c xhook/xh_util.c xhook/xh_version.c)
target_include_directories(xhook PUBLIC ${D}/xhook)
target_link_libraries(xhook PUBLIC ${log-lib})

# =============================================================================
# And64Hook — inline hook arm64 (A64HookFunction)
# =============================================================================
add_library(And64Hook STATIC And64Hook/And64Hook.cpp)
target_include_directories(And64Hook PUBLIC ${D}/And64Hook)
target_compile_features(And64Hook PUBLIC cxx_std_17)
target_link_libraries(And64Hook PUBLIC ${log-lib})

# =============================================================================
# AntiLibPatch — chống vá .text qua ElfImg (Logger/obfuscate ở Includes/)
# =============================================================================
add_library(antilibpatch STATIC
    AntiLibPatch/AntiLibPatch.cpp AntiLibPatch/ElfImg.cpp)
target_include_directories(antilibpatch PUBLIC ${D}/AntiLibPatch ${D}/Includes)
target_compile_features(antilibpatch PUBLIC cxx_std_20)
target_link_libraries(antilibpatch PUBLIC lgl ${log-lib})

# =============================================================================
# apksig — APK Signature Scheme v2/v3 verifier (#include "apksig/...")
# NOTE: các .cpp dùng OpenSSL/BoringSSL (openssl/sha.h, evp.h, x509.h, pkcs7.h,
#       asn1.h…). Project KHÔNG có thư mục header openssl/ nào — chỉ libcurl_all.a
#       (boringssl ssl+crypto) cung cấp SYMBOL lúc link, KHÔNG có HEADER lúc
#       compile. Phải bổ sung header OpenSSL/BoringSSL (vd: trỏ include dir tới
#       boringssl/include) thì target này mới compile được. Link curl ở dưới để
#       lấy symbol crypto đã merge sẵn.
# =============================================================================
add_library(apksig STATIC
    apksig/apk_reader.cpp apksig/apk_verifier.cpp apksig/certificate_extractor.cpp
    apksig/digest_verifier.cpp apksig/signing_block_locator.cpp
    apksig/signing_block_parser.cpp apksig/v2_v3_parser.cpp)
target_include_directories(apksig PUBLIC ${D} ${D}/apksig ${D}/Includes)
target_compile_features(apksig PUBLIC cxx_std_17)
target_link_libraries(apksig PUBLIC curl lgl ${log-lib})

# =============================================================================
# json — nlohmann/json header-only (INTERFACE)
# =============================================================================
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${D}/json)

# =============================================================================
# gameapi — IL2CPP runtime API + UnityEngine wrappers (#include "API/...")
# =============================================================================
file(GLOB_RECURSE _gameapi_src ${D}/API/*.cpp)
add_library(gameapi STATIC ${_gameapi_src})
target_include_directories(gameapi PUBLIC
    ${D} ${D}/API ${D}/API/game ${D}/Includes)
target_compile_features(gameapi PUBLIC cxx_std_20)
target_link_libraries(gameapi PUBLIC xdl lgl tools ${log-lib})

# =============================================================================
# Tools — hook/memory helpers: DobbyHook, maps base/end, pkg+apk path, sleep
#   includes "Logger.h"/<obfuscate.h> (${D}/Includes), And64Hook/xdl/Dobby (${D})
# =============================================================================
add_library(tools STATIC Tools/Tools.cpp)
target_include_directories(tools PUBLIC ${D}/Tools ${D} ${D}/Includes)
target_compile_features(tools PUBLIC cxx_std_20)
target_link_libraries(tools PUBLIC dobby lgl ${log-lib})
