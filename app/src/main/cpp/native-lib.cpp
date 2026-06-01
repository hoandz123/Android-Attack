#include <jni.h>
#include <android/log.h>

#define LOG_TAG "AttackNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {

jstring stringFromNativeImpl(JNIEnv *env, jobject /* thiz */) {
    LOGI("attack module invoked");
    return env->NewStringUTF("Hello from C++ (NDK 28)");
}

const JNINativeMethod kMainActivityMethods[] = {
    {"stringFromNative", "()Ljava/lang/String;", reinterpret_cast<void *>(stringFromNativeImpl)},
};

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /* reserved */) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass clazz = env->FindClass("com/android/attack/MainActivity");
    if (clazz == nullptr) {
        return JNI_ERR;
    }

    if (env->RegisterNatives(clazz, kMainActivityMethods, sizeof(kMainActivityMethods) / sizeof(kMainActivityMethods[0])) != JNI_OK) {
        return JNI_ERR;
    }

    LOGI("RegisterNatives complete");
    return JNI_VERSION_1_6;
}
