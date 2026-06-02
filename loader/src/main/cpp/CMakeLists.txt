cmake_minimum_required(VERSION 3.22.1)

project("loader")

file(TO_CMAKE_PATH "${NATIVE_CORE_CPP}" _nc)
set(_dobby "${_nc}/dobby/${ANDROID_ABI}/libdobby.a")
if(NOT EXISTS ${_dobby})
    message(FATAL_ERROR "Missing ${_dobby}")
endif()

add_library(loader SHARED
    Loader.cpp
    Zygisk/zygisk.cpp
    ${_nc}/IncludesLink.cpp
    ${_nc}/Includes/oxorany.cpp
    ${_nc}/dobby/DobbyLink.cpp
    ${_nc}/xdl/xdl.c
    ${_nc}/xdl/xdl_iterate.c
    ${_nc}/xdl/xdl_linker.c
    ${_nc}/xdl/xdl_lzma.c
    ${_nc}/xdl/xdl_util.c
)

find_library(log-lib log)

target_include_directories(loader PRIVATE
    ${_nc}
    ${_nc}/Includes
    ${_nc}/xdl
    ${_nc}/dobby
    ${CMAKE_CURRENT_SOURCE_DIR}/Zygisk
)

target_link_libraries(loader ${log-lib} dl ${_dobby})

target_compile_features(loader PRIVATE cxx_std_20)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(loader PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(loader PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
        "LINKER:--exclude-libs,ALL"
        "LINKER:--strip-all"
        "LINKER:--gc-sections")
endif()
