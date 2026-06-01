#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>
#include <string>

#define TAG "AttackLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static const char *kPlugins[] = {"libattack.so"};
static void *s_plugin_handle = nullptr;

static bool LoadPlugin(JavaVM *vm, void *reserved, const char *path) {
    if (s_plugin_handle) return true;
    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) { LOGE("dlopen %s: %s", path, dlerror()); return false; }
    auto onLoad = (jint (*)(JavaVM *, void *))dlsym(handle, "JNI_OnLoad");
    if (!onLoad) { LOGE("dlsym JNI_OnLoad: %s", dlerror()); return false; }
    if (onLoad(vm, reserved) != JNI_VERSION_1_6) {
        LOGE("JNI_OnLoad failed");
        dlclose(handle);
        return false;
    }
    s_plugin_handle = handle;
    LOGI("loaded plugin %s", path);
    return true;
}

static bool LoadPlugins(JavaVM *vm, void *reserved, const std::string &dir) {
    for (const char *name : kPlugins) {
        // APK libs are often mmap'd from base.apk (extractNativeLibs=false); basename works.
        if (LoadPlugin(vm, reserved, name)) continue;
        if (!dir.empty()) {
            std::string path = dir + "/" + name;
            if (LoadPlugin(vm, reserved, path.c_str())) continue;
        }
        LOGE("failed to load plugin %s", name);
        return false;
    }
    return true;
}

static bool NativeLibDir(JNIEnv *env, std::string &out) {
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
    NativeLibDir(env, dir);
    return LoadPlugins(vm, reserved, dir) ? JNI_VERSION_1_6 : JNI_ERR;
}
