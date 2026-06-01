#include <jni.h>

static jstring stringFromNative(JNIEnv *env, jobject) { return env->NewStringUTF("Hello from C++ (NDK 28)"); }

extern "C" jboolean attack_register_natives(JNIEnv *env) {
    JNINativeMethod m = {"stringFromNative", "()Ljava/lang/String;", (void *)stringFromNative};
    jclass c = env->FindClass("com/android/attack/MainActivity");
    if (!c) return JNI_FALSE;
    return env->RegisterNatives(c, &m, 1) == JNI_OK ? JNI_TRUE : JNI_FALSE;
}
