cmake_minimum_required(VERSION 3.22.1)
project("attack")

find_package(native-core REQUIRED CONFIG)
find_package(mod-ui REQUIRED CONFIG)

include(${NATIVE_CORE_CPP}/cmake/PrefabStatic.cmake)

add_library(attack SHARED native-lib.cpp app_menu.cpp)
if(EMBEDDED_DEX_DIR AND IS_DIRECTORY "${EMBEDDED_DEX_DIR}")
    target_include_directories(attack PRIVATE "${EMBEDDED_DEX_DIR}")
    set(_embed_hpp "${EMBEDDED_DEX_DIR}/embedded_dex.hpp")
    if(EXISTS "${_embed_hpp}")
        set_source_files_properties(native-lib.cpp PROPERTIES OBJECT_DEPENDS "${_embed_hpp}")
    endif()
    message(STATUS "attack: embedded dex dir ${EMBEDDED_DEX_DIR}")
else()
    message(WARNING "attack: EMBEDDED_DEX_DIR missing — chạy generateEmbeddedDex / preBuild")
endif()

find_library(log-lib log)
find_library(z-lib z)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

target_include_directories(attack PRIVATE
    ${NATIVE_CORE_CPP}
    ${NATIVE_CORE_CPP}/imgui
    ${NATIVE_CORE_CPP}/imgui/backends)

target_link_libraries(attack PRIVATE
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib}
    ${z-lib}
    mod-ui::modui
    native-core::kitty
    native-core::dexloader
    native-core::activitytracker
    native-core::dobby
    native-core::curl
    native-core::httpclient
)

link_prefab_static(attack ${MOD_UI_PREFAB} modui)
link_prefab_static(attack ${NATIVE_CORE_PREFAB} imgui jnihelper dexloader activitytracker httpclient filemanager)
link_native_core_prebuilts(attack ${NATIVE_CORE_CPP})

target_compile_features(attack PRIVATE cxx_std_20)
target_compile_definitions(attack PRIVATE CURL_STATICLIB)
