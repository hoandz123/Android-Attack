cmake_minimum_required(VERSION 3.22.1)

project("loader")

add_library(loader SHARED Loader.cpp)

find_library(log-lib log)

target_include_directories(loader PRIVATE ${NATIVE_CORE_CPP})
target_link_libraries(loader ${log-lib} dl)

target_compile_features(loader PRIVATE cxx_std_20)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(loader PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(loader PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map"
        "LINKER:--exclude-libs,ALL"
        "LINKER:--strip-all"
        "LINKER:--gc-sections")
endif()
