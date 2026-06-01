cmake_minimum_required(VERSION 3.22.1)
project(mod-ui)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(native-core REQUIRED CONFIG)

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)

add_library(modui STATIC
    mod_ui.cpp
    mod_ui_font.cpp
    mod_ui_theme.cpp
    mod_ui_layout.cpp
    mod_ui_shell.cpp
    mod_ui_icon.cpp
    mod_ui_render.cpp
    mod_ui_touch.cpp
    mod_ui_keyboard.cpp
    mod_ui_android_keys.cpp
    surface_bridge.cpp)

target_include_directories(modui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    ${NATIVE_CORE_CPP}
    ${NATIVE_CORE_CPP}/imgui/backends)

target_compile_definitions(modui PUBLIC IMGUI_IMPL_OPENGL_ES3)

target_include_directories(modui PRIVATE
    ${NATIVE_CORE_CPP}/HttpClient
    ${NATIVE_CORE_CPP}/FileManager)

target_link_libraries(modui PUBLIC
    native-core::jnihelper
    native-core::imgui
    native-core::httpclient
    native-core::filemanager
    native-core::lgl
    ${log-lib}
    ${android-lib}
    ${EGL-lib}
    ${GLESv3-lib})

target_compile_features(modui PUBLIC cxx_std_20)
