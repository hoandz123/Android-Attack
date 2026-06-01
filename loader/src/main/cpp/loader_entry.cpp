#include "native_loader.hpp"

#include <android/log.h>
#include <jni.h>

#define LOG_TAG "AttackLoader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

jboolean nativeLoadFromDir(JNIEnv *env, jclass /*clazz*/, jstring nativeLibraryDir, jstring libName) {
    if (nativeLibraryDir == nullptr || libName == nullptr) {
        LOGE("nativeLoadFromDir: null argument");
        return JNI_FALSE;
    }

    const char *dir = env->GetStringUTFChars(nativeLibraryDir, nullptr);
    const char *name = env->GetStringUTFChars(libName, nullptr);
    const bool ok = attack::loader::loadFromDir(dir, name);

    if (dir != nullptr) {
        env->ReleaseStringUTFChars(nativeLibraryDir, dir);
    }
    if (name != nullptr) {
        env->ReleaseStringUTFChars(libName, name);
    }

    return ok ? JNI_TRUE : JNI_FALSE;
}

jboolean nativeLoadDownloaded(JNIEnv *env, jclass /*clazz*/, jstring absolutePath) {
    if (absolutePath == nullptr) {
        LOGE("nativeLoadDownloaded: null path");
        return JNI_FALSE;
    }

    const char *path = env->GetStringUTFChars(absolutePath, nullptr);
    const bool ok = attack::loader::loadDownloaded(path);

    if (path != nullptr) {
        env->ReleaseStringUTFChars(absolutePath, path);
    }

    return ok ? JNI_TRUE : JNI_FALSE;
}

const JNINativeMethod kNativeMethods[] = {
    {"nativeLoadFromDir", "(Ljava/lang/String;Ljava/lang/String;)Z", reinterpret_cast<void *>(nativeLoadFromDir)},
    {"nativeLoadDownloaded", "(Ljava/lang/String;)Z", reinterpret_cast<void *>(nativeLoadDownloaded)},
};

bool registerNatives(JNIEnv *env) {
    jclass clazz = env->FindClass("com/android/attack/loader/NativeLoader");
    if (clazz == nullptr) {
        LOGE("FindClass NativeLoader failed");
        return false;
    }

    if (env->RegisterNatives(clazz, kNativeMethods, sizeof(kNativeMethods) / sizeof(kNativeMethods[0])) != JNI_OK) {
        LOGE("RegisterNatives failed");
        return false;
    }

    return true;
}

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (!attack::loader::bootstrap(vm, reserved)) {
        return JNI_ERR;
    }

    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    if (!registerNatives(env)) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
