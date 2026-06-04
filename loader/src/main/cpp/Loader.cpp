#include <dlfcn.h>
#include <jni.h>
#include <string>

#define LOGGER_TAG "ATTACK_Loader"
#include <Includes/Logger.h>
#include <Tools/Tools.h>

static const char *kPlugins[] = {"libattack.so"};
static void *s_plugin_handle = nullptr;

static bool LoadPlugin(JavaVM *vm, void *reserved, const char *path) {
    LOGI(OBF("LoadPlugin path=%s"), path);
    if (s_plugin_handle) return true;
    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) { LOGE(OBF("dlopen %s: %s"), path, dlerror()); return false; }
    auto onLoad = (jint (*)(JavaVM *, void *))dlsym(handle, OBF("JNI_OnLoad"));
    if (!onLoad) { LOGE(OBF("dlsym JNI_OnLoad: %s"), dlerror()); return false; }
    if (onLoad(vm, reserved) != JNI_VERSION_1_6) {
        LOGE(OBF("JNI_OnLoad failed"));
        dlclose(handle);
        return false;
    }
    s_plugin_handle = handle;
    LOGI(OBF("loaded plugin %s"), path);
    return true;
}

static bool LoadPlugins(JavaVM *vm, void *reserved, const std::string &dir) {
    LOGI(OBF("LoadPlugins dir=%s"), dir.c_str());
    for (const char *name : kPlugins) {
        // APK libs are often mmap'd from base.apk (extractNativeLibs=false); basename works.
        if (LoadPlugin(vm, reserved, name)) continue;
        if (!dir.empty()) {
            std::string path = dir + OBFS("/") + name;
            if (LoadPlugin(vm, reserved, path.c_str())) continue;
        }
        LOGE(OBF("failed to load plugin %s"), name);
        return false;
    }
    return true;
}

static bool AppDataDir(std::string &out) {
    std::string pkg = Tools::GetPackageName();
    if (pkg.empty()) return false;
    out = OBF("/data/user/0/") + pkg;
    return true;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    LOGI(OBF("JNI_OnLoad"));
    std::string dir;
    if (!AppDataDir(dir)) LOGE(OBF("AppDataDir failed"));
    return LoadPlugins(vm, reserved, dir) ? JNI_VERSION_1_6 : JNI_ERR;
}
