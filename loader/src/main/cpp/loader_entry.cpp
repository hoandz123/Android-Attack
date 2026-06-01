#include "native_loader.hpp"

#include <jni.h>

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (!attack::loader::bootstrap(vm, reserved)) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
