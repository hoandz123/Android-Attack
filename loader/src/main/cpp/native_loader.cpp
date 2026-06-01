#include "native_loader.hpp"

#include <android/log.h>
#include <dlfcn.h>

#include <cstring>
#include <string>

#define LOG_TAG "AttackLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

JavaVM *g_vm = nullptr;

using RegisterPluginNativesFn = jboolean (*)(JNIEnv *);

bool registerPluginNatives(JNIEnv *env, void *handle) {
    if (env == nullptr || handle == nullptr) {
        LOGE("registerPluginNatives: invalid args");
        return false;
    }

    dlerror();
    auto registerFn = reinterpret_cast<RegisterPluginNativesFn>(dlsym(handle, "attack_register_natives"));
    const char *error = dlerror();
    if (error != nullptr || registerFn == nullptr) {
        LOGE("dlsym attack_register_natives failed: %s", error != nullptr ? error : "null symbol");
        return false;
    }

    if (registerFn(env) != JNI_TRUE) {
        LOGE("attack_register_natives returned false");
        return false;
    }

    return true;
}

bool dlopenAbsolute(JNIEnv *env, const char *path, bool required) {
    if (path == nullptr || path[0] == '\0') {
        LOGE("empty library path");
        return !required;
    }

    dlerror();
    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (handle == nullptr) {
        LOGE("dlopen failed: %s (%s)", path, dlerror());
        return !required;
    }

    LOGI("loaded %s", path);
    if (!registerPluginNatives(env, handle)) {
        return false;
    }

    return true;
}

std::string joinPath(const char *dir, const char *fileName) {
    std::string path(dir);
    if (!path.empty() && path.back() != '/') {
        path += '/';
    }
    path += fileName;
    return path;
}

std::string normalizeLibFileName(const char *libName) {
    std::string fileName = libName;
    if (fileName.rfind("lib", 0) != 0) {
        fileName = "lib" + fileName;
    }
    if (fileName.size() < 3 || fileName.substr(fileName.size() - 3) != ".so") {
        fileName += ".so";
    }
    return fileName;
}

bool resolveNativeLibraryDir(JNIEnv *env, std::string &outDir) {
    if (env == nullptr) {
        return false;
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (activityThreadClass == nullptr) {
        return false;
    }

    jmethodID currentApplication = env->GetStaticMethodID(activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    if (currentApplication == nullptr) {
        return false;
    }

    jobject application = env->CallStaticObjectMethod(activityThreadClass, currentApplication);
    if (application == nullptr) {
        return false;
    }

    jclass applicationClass = env->GetObjectClass(application);
    jmethodID getApplicationInfo = env->GetMethodID(applicationClass, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    if (getApplicationInfo == nullptr) {
        return false;
    }

    jobject applicationInfo = env->CallObjectMethod(application, getApplicationInfo);
    if (applicationInfo == nullptr) {
        return false;
    }

    jfieldID nativeLibraryDirField = env->GetFieldID(env->GetObjectClass(applicationInfo), "nativeLibraryDir", "Ljava/lang/String;");
    if (nativeLibraryDirField == nullptr) {
        return false;
    }

    auto nativeLibraryDir = reinterpret_cast<jstring>(env->GetObjectField(applicationInfo, nativeLibraryDirField));
    if (nativeLibraryDir == nullptr) {
        return false;
    }

    const char *dir = env->GetStringUTFChars(nativeLibraryDir, nullptr);
    if (dir == nullptr) {
        return false;
    }

    outDir = dir;
    env->ReleaseStringUTFChars(nativeLibraryDir, dir);
    return true;
}

} // namespace

namespace attack::loader {

bool bootstrap(JavaVM *vm, void * /*reserved*/) {
    g_vm = vm;
    LOGI("JNI_OnLoad — client entry");

    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("GetEnv failed");
        return false;
    }

    return loadBuiltinPlugins(env);
}

bool loadBuiltinPlugins(JNIEnv *env) {
    std::string nativeLibraryDir;
    if (resolveNativeLibraryDir(env, nativeLibraryDir)) {
        LOGI("nativeLibraryDir=%s", nativeLibraryDir.c_str());
        return loadFromDir(env, nativeLibraryDir.c_str(), "attack");
    }

    LOGI("Application not ready, fallback dlopen by name");
    return dlopenAbsolute(env, normalizeLibFileName("attack").c_str(), true);
}

bool loadFromDir(JNIEnv *env, const char *nativeLibraryDir, const char *libName) {
    if (env == nullptr || nativeLibraryDir == nullptr || libName == nullptr) {
        LOGE("loadFromDir: invalid args");
        return false;
    }

    return dlopenAbsolute(env, joinPath(nativeLibraryDir, normalizeLibFileName(libName).c_str()).c_str(), true);
}

bool loadDownloaded(JNIEnv *env, const char *absolutePath) {
    if (env == nullptr || absolutePath == nullptr) {
        LOGE("loadDownloaded: invalid args");
        return false;
    }

    return dlopenAbsolute(env, absolutePath, true);
}

} // namespace attack::loader
