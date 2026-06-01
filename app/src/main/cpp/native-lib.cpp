#include <jni.h>

static jstring stringFromNative(JNIEnv *env, jobject) { return env->NewStringUTF("Hello from C++ (NDK 28)"); }

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    JNINativeMethod m = {"stringFromNative", "()Ljava/lang/String;", (void *)stringFromNative};
    jclass c = env->FindClass("com/android/attack/MainActivity");
    if (!c || env->RegisterNatives(c, &m, 1) != JNI_OK) return JNI_ERR;
    return JNI_VERSION_1_6;
}
