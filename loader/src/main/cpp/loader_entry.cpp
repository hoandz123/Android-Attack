#include "native_loader.hpp"

#include <android/log.h>
#include <jni.h>

#define LOG_TAG "AttackLoader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (!attack::loader::bootstrap(vm, reserved)) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL Java_com_android_attack_MainActivity_bootstrap(JNIEnv *env, jclass /*clazz*/, jstring nativeLibraryDir) {
    if (nativeLibraryDir == nullptr) {
        LOGE("bootstrap: null nativeLibraryDir");
        return;
    }

    const char *dir = env->GetStringUTFChars(nativeLibraryDir, nullptr);
    if (dir == nullptr) {
        LOGE("bootstrap: GetStringUTFChars failed");
        return;
    }

    if (!attack::loader::loadFromDir(env, dir, "attack")) {
        LOGE("bootstrap: failed to load libattack.so");
    }

    env->ReleaseStringUTFChars(nativeLibraryDir, dir);
}
