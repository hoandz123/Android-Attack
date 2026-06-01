#include <jni.h>
#include <android/log.h>

#define LOG_TAG "AttackNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

jstring stringFromNativeImpl(JNIEnv *env, jobject /* thiz */) {
    LOGI("attack module invoked");
    return env->NewStringUTF("Hello from C++ (NDK 28)");
}

const JNINativeMethod kMainActivityMethods[] = {
    {"stringFromNative", "()Ljava/lang/String;", reinterpret_cast<void *>(stringFromNativeImpl)},
};

} // namespace

extern "C" JNIEXPORT jboolean JNICALL attack_register_natives(JNIEnv *env) {
    if (env == nullptr) {
        return JNI_FALSE;
    }

    jclass clazz = env->FindClass("com/android/attack/MainActivity");
    if (clazz == nullptr) {
        LOGE("FindClass MainActivity failed");
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, kMainActivityMethods, sizeof(kMainActivityMethods) / sizeof(kMainActivityMethods[0])) != JNI_OK) {
        LOGE("RegisterNatives failed");
        return JNI_FALSE;
    }

    LOGI("RegisterNatives complete");
    return JNI_TRUE;
}
