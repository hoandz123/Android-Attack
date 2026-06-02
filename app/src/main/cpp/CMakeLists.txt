cmake_minimum_required(VERSION 3.22.1)
project("attack")

find_package(native-core REQUIRED CONFIG)
find_package(mod-ui REQUIRED CONFIG)

include(${NATIVE_CORE_CPP}/cmake/PrefabStatic.cmake)

set(PLAY_TOGETHER_SOURCES
    PlayTogether/PlayTogether.cpp
    PlayTogether/Hook/Hook.cpp
    PlayTogether/Hook/AntiCheat.cpp
    PlayTogether/Config/Config.cpp
    PlayTogether/UI/MenuUi.cpp
    PlayTogether/Hook/AutoFishing/AutoFishing.cpp
    PlayTogether/Hook/AutoFishing/PickerSnapshot.cpp
    PlayTogether/Hook/SDK/FrameWork.cpp
    PlayTogether/Hook/SDK/SystemHelper.cpp
    PlayTogether/Hook/SDK/CacheSystem.cpp
    PlayTogether/Hook/SDK/CacheUser.cpp
    PlayTogether/Hook/SDK/ActorControl.cpp
    PlayTogether/Hook/SDK/TableSystem.cpp
    PlayTogether/Hook/SDK/TableItemImpl.cpp
    PlayTogether/Hook/SDK/TableMessagesImpl.cpp
    PlayTogether/Hook/SDK/FishingSystem.cpp
    PlayTogether/Hook/SDK/TableFishingDifficultyImpl.cpp
    PlayTogether/Hook/SDK/TableFishingAreaImpl.cpp
    PlayTogether/Hook/SDK/TableFishingBaitImpl.cpp)

add_library(attack SHARED
    NativeLib.cpp Menu.cpp Games.cpp
    LienQuan/LienQuan.cpp
    ${PLAY_TOGETHER_SOURCES})
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
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayTogether/Hook/SDK
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayTogether/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayTogether/UI
    ${CMAKE_CURRENT_SOURCE_DIR}/PlayTogether/Hook
    ${NATIVE_CORE_CPP}
    ${NATIVE_CORE_CPP}/Includes
    ${NATIVE_CORE_CPP}/Tools
    ${NATIVE_CORE_CPP}/API
    ${NATIVE_CORE_CPP}/API/game
    ${NATIVE_CORE_CPP}/FileManager
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
link_prefab_static(attack ${NATIVE_CORE_PREFAB} kitty lgl imgui jnihelper dexloader activitytracker httpclient filemanager tools xdl gameapi base64 sharedprefs gameui)
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
