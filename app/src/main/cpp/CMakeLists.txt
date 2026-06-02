cmake_minimum_required(VERSION 3.22.1)
project("attack")

find_package(native-core REQUIRED CONFIG)
find_package(mod-ui REQUIRED CONFIG)

include(${NATIVE_CORE_CPP}/cmake/PrefabStatic.cmake)

add_library(attack SHARED
    NativeLib.cpp Menu.cpp Games.cpp
    LienQuan/LienQuan.cpp
    PlayTogether/PlayTogether.cpp
    PlayTogether/HOOK_PLAY/HOOK_PLAY.cpp
    PlayTogether/HOOK_PLAY/AntiCheat.cpp
    PlayTogether/SDK/FrameWork.cpp
    PlayTogether/SDK/SystemHelper.cpp
    PlayTogether/SDK/CacheSystem.cpp
    PlayTogether/SDK/CacheUser.cpp
    PlayTogether/SDK/ActorControl.cpp
    PlayTogether/SDK/KinematicCharacterMotor.cpp)
if(EMBEDDED_DEX_DIR)
    target_include_directories(attack PRIVATE "${EMBEDDED_DEX_DIR}")
    set(_embed_hpp "${EMBEDDED_DEX_DIR}/embedded_dex.hpp")
    if(EXISTS "${_embed_hpp}")
        set_source_files_properties(NativeLib.cpp PROPERTIES OBJECT_DEPENDS "${_embed_hpp}")
    endif()
    message(STATUS "attack: embedded dex dir ${EMBEDDED_DEX_DIR}")
else()
    message(WARNING "attack: EMBEDDED_DEX_DIR not set — chạy generateEmbeddedDex / preBuild")
endif()

find_library(log-lib log)
find_library(z-lib z)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

target_include_directories(attack PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayTogether
    ${NATIVE_CORE_CPP}
    ${NATIVE_CORE_CPP}/Includes
    ${NATIVE_CORE_CPP}/Tools
    ${NATIVE_CORE_CPP}/API
    ${NATIVE_CORE_CPP}/API/game
    ${NATIVE_CORE_CPP}/json
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
link_prefab_static(attack ${NATIVE_CORE_PREFAB} imgui jnihelper dexloader activitytracker httpclient filemanager tools xdl gameapi)
link_native_core_prebuilts(attack ${NATIVE_CORE_CPP})

target_compile_features(attack PRIVATE cxx_std_20)
target_compile_definitions(attack PRIVATE CURL_STATICLIB)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(attack PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(attack PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
        "LINKER:--exclude-libs,ALL"
        "LINKER:--strip-all"
        "LINKER:--gc-sections")
endif()
