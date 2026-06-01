#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "AttackNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_android_attack_MainActivity_stringFromNative(
    JNIEnv *env,
    jobject /* this */) {
    LOGI("attack module invoked");
    return env->NewStringUTF("Hello from C++ (NDK 28)");
}
