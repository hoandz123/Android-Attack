#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "AttackNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_android_attack_MainActivity_stringFromNative(
    JNIEnv *env,
    jobject /* this */) {
    LOGI("Native library loaded");
    return env->NewStringUTF("Hello from C++ (NDK 28)");
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /* reserved */) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    LOGI("JNI_OnLoad");
    return JNI_VERSION_1_6;
}
