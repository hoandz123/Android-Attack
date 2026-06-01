#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <string>

#define TAG "AttackLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool loadPlugin(JavaVM *vm, void *reserved, const char *path) {
    if (!vm || !path || !*path) return false;
    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) { LOGE("dlopen %s: %s", path, dlerror()); return false; }
    auto onLoad = (jint (*)(JavaVM *, void *))dlsym(handle, "JNI_OnLoad");
    if (!onLoad) { LOGE("dlsym JNI_OnLoad %s: %s", path, dlerror()); return false; }
    if (onLoad(vm, reserved) != JNI_VERSION_1_6) { LOGE("JNI_OnLoad failed: %s", path); return false; }
    LOGI("loaded %s", path);
    return true;
}

static bool nativeLibDir(JNIEnv *env, std::string &out) {
    jclass at = env->FindClass("android/app/ActivityThread");
    jmethodID app = env->GetStaticMethodID(at, "currentApplication", "()Landroid/app/Application;");
    jobject ctx = env->CallStaticObjectMethod(at, app);
    if (!ctx) return false;
    jmethodID info = env->GetMethodID(env->GetObjectClass(ctx), "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    jobject ai = env->CallObjectMethod(ctx, info);
    if (!ai) return false;
    auto dir = (jstring)env->GetObjectField(ai, env->GetFieldID(env->GetObjectClass(ai), "nativeLibraryDir", "Ljava/lang/String;"));
    if (!dir) return false;
    const char *utf = env->GetStringUTFChars(dir, nullptr);
    if (!utf) return false;
    out = utf;
    env->ReleaseStringUTFChars(dir, utf);
    return true;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    LOGI("JNI_OnLoad");
    std::string dir;
    if (nativeLibDir(env, dir)) return loadPlugin(vm, reserved, (dir + "/libattack.so").c_str()) ? JNI_VERSION_1_6 : JNI_ERR;
    return loadPlugin(vm, reserved, "libattack.so") ? JNI_VERSION_1_6 : JNI_ERR;
}
