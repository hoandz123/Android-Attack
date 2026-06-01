# AGP Prefab often exports headers-only targets; link packaged .a from prefab/modules.
function(link_prefab_static target prefab_root)
    set(_abi "android.${ANDROID_ABI}")
    set(_modules ${ARGN})
    foreach(_mod IN LISTS _modules)
        set(_lib "${prefab_root}/modules/${_mod}/libs/${_abi}/lib${_mod}.a")
        target_link_libraries(${target} PRIVATE "${_lib}")
    endforeach()
endfunction()

function(link_native_core_prebuilts target native_core_cpp)
    target_link_libraries(${target} PRIVATE
        "${native_core_cpp}/dobby/${ANDROID_ABI}/libdobby.a"
        "${native_core_cpp}/curl/${ANDROID_ABI}/libcurl_all.a")
endfunction()
