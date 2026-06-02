#include <algorithm>
#include <array>
#include <cerrno>
#include <cinttypes>
#include <cstdarg>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <dlfcn.h>
#include <linux/unistd.h>
#include <android/api-level.h>

#include "zygisk.hpp"
#include <Includes/obfuscate.h>

#define LOG_TAG OBF("ATTACK_LoaderZygisk")
#include <Includes/Logger.h>

#include "xdl/xdl.h"
#include "dobby/dobby.h"

extern "C" jint JNI_OnLoad(JavaVM *vm, void *reserved);

void hack_prepare(const char *game_data_dir);

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(package_name, app_data_dir);
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (!enable_hack) return;
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
        JavaVM *vms;
        env->GetJavaVM(&vms);
        JNI_OnLoad(vms, (void *)game_data_dir);
#else
        std::thread hack_thread(hack_prepare, game_data_dir);
        hack_thread.detach();
#endif
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack = false;
    char *game_data_dir = nullptr;

    void preSpecialize(const char *package_name, const char *app_data_dir) {
        std::vector<std::string> target_packages = {OBF("com.vng.playtogether"), OBF("com.haegin.playtogether")};
        if (std::find(target_packages.begin(), target_packages.end(), std::string(package_name)) == target_packages.end()) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        LOGI(OBF("preSpecialize detect game: %s"), package_name);
        enable_hack = true;
        game_data_dir = new char[strlen(app_data_dir) + 1];
        strcpy(game_data_dir, app_data_dir);
    }
};

std::string GetLibDir(JavaVM *vms) {
    JNIEnv *env = nullptr;
    if (vms->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        LOGE(OBF("Failed to attach thread"));
        return {};
    }
    jclass activity_thread_clz = env->FindClass(OBF("android/app/ActivityThread"));
    if (!activity_thread_clz) {
        LOGE(OBF("ActivityThread class not found"));
        return {};
    }
    jmethodID current_application_id = env->GetStaticMethodID(activity_thread_clz, OBF("currentApplication"), OBF("()Landroid/app/Application;"));
    if (!current_application_id) {
        LOGE(OBF("currentApplication method not found"));
        return {};
    }
    jobject application = env->CallStaticObjectMethod(activity_thread_clz, current_application_id);
    if (!application) {
        LOGE(OBF("Application instance not found"));
        return {};
    }
    jclass application_clazz = env->GetObjectClass(application);
    jmethodID get_application_info = env->GetMethodID(application_clazz, OBF("getApplicationInfo"), OBF("()Landroid/content/pm/ApplicationInfo;"));
    if (!get_application_info) {
        LOGE(OBF("getApplicationInfo method not found"));
        return {};
    }
    jobject application_info = env->CallObjectMethod(application, get_application_info);
    jclass application_info_clazz = env->GetObjectClass(application_info);
    jfieldID native_library_dir_id = env->GetFieldID(application_info_clazz, OBF("nativeLibraryDir"), OBF("Ljava/lang/String;"));
    if (!native_library_dir_id) {
        LOGE(OBF("nativeLibraryDir field not found"));
        return {};
    }
    jstring native_library_dir_jstring = (jstring)env->GetObjectField(application_info, native_library_dir_id);
    if (!native_library_dir_jstring) {
        LOGE(OBF("nativeLibraryDir value not found"));
        return {};
    }
    const char *path = env->GetStringUTFChars(native_library_dir_jstring, nullptr);
    std::string lib_dir(path);
    env->ReleaseStringUTFChars(native_library_dir_jstring, path);
    LOGI(OBF("Library directory: %s"), lib_dir.c_str());
    return lib_dir;
}

static std::string GetNativeBridgeLibrary() {
    auto value = std::array<char, 255>();
    __system_property_get(OBF("ro.dalvik.vm.native.bridge"), value.data());
    return {value.data()};
}

struct NativeBridgeCallbacks {
    uint32_t version;
    void *initialize;
    void *(*loadLibrary)(const char *libpath, int flag);
    void *(*getTrampoline)(void *handle, const char *name, const char *shorty, uint32_t len);
    void *isSupported;
    void *getAppEnv;
    void *isCompatibleWith;
    void *getSignalHandler;
    void *unloadLibrary;
    void *getError;
    void *isPathSupported;
    void *initAnonymousNamespace;
    void *createNamespace;
    void *linkNamespaces;
    void *(*loadLibraryExt)(const char *libpath, int flag, void *ns);
};

bool NativeBridgeLoad(const char *game_data_dir, int api_level, void *data, size_t length) {
    auto libart = dlopen(OBF("libart.so"), RTLD_NOW);
    auto JNI_GetCreatedJavaVMs = (jint (*)(JavaVM **, jsize, jsize *))dlsym(libart, OBF("JNI_GetCreatedJavaVMs"));
    JavaVM *vms_buf[1];
    JavaVM *vms;
    jsize num_vms;
    jint status = JNI_GetCreatedJavaVMs(vms_buf, 1, &num_vms);
    if (status == JNI_OK && num_vms > 0) {
        vms = vms_buf[0];
    } else {
        LOGE(OBF("GetCreatedJavaVMs error"));
        return false;
    }
    auto lib_dir = GetLibDir(vms);
    if (lib_dir.empty()) return false;
    if (lib_dir.find(OBF("/lib/x86")) != std::string::npos) {
        munmap(data, length);
        return false;
    }
    auto nb = dlopen(OBF("libhoudini.so"), RTLD_NOW);
    if (!nb) {
        auto native_bridge = GetNativeBridgeLibrary();
        nb = dlopen(native_bridge.data(), RTLD_NOW);
    }
    if (!nb) return false;
    auto callbacks = (NativeBridgeCallbacks *)dlsym(nb, OBF("NativeBridgeItf"));
    if (!callbacks) return false;
    int fd = syscall(__NR_memfd_create, OBF("anon"), MFD_CLOEXEC);
    ftruncate(fd, (off_t)length);
    void *mem = mmap(nullptr, length, PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(mem, data, length);
    munmap(mem, length);
    munmap(data, length);
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, OBF("/proc/self/fd/%d"), fd);
    void *arm_handle;
    if (api_level >= 26) {
        arm_handle = callbacks->loadLibraryExt(path, RTLD_NOW, (void *)3);
    } else {
        arm_handle = callbacks->loadLibrary(path, RTLD_NOW);
    }
    if (!arm_handle) {
        close(fd);
        return false;
    }
    LOGI(OBF("Native bridge load success"));
    auto init = (void (*)(JavaVM *, void *))callbacks->getTrampoline(arm_handle, OBF("JNI_OnLoad"), nullptr, 0);
    if (!init) {
        LOGE(OBF("Get JNI_OnLoad trampoline failed"));
        exit(-1);
    }
    init(vms, (void *)game_data_dir);
    return true;
}

static const char *const g_accessBlockList[] = {
    OBF("/data/data/com.vng.playtogether/cache/base.apk"),
    OBF("/system/app/gpLogin/gpLogin.apk"),
    OBF("/system/app/gpLogin/gpLogin_new.apk"),
    OBF("/system/app/Helper/helper.apk"),
    OBF("/system/app/Helper/NoxHelp_en.apk"),
    OBF("/system/app/Helper/NoxHelp_en_new.apk"),
    OBF("/bin/nox-vbox-sf"),
    OBF("/bin/noxd"),
    OBF("/data/app/com.android.ld.appstore-1/base.apk"),
    OBF("/data/app/com.android.ld.appstore-2/base.apk"),
    OBF("/system/app/Launcher3/Launcher3.apk"),
    OBF("/system/priv-app/LDAppStore/LDAppStore.apk"),
    OBF("/data/user_de/0/com.android.flysilkworm"),
    OBF("/data/misc/profiles/ref/com.android.flysilkworm"),
    OBF("/system/bin/bstfolderd"),
    OBF("/system/bin/bstfolder_ctl"),
    OBF("/system/bin/bstsyncfs"),
    OBF("/system/bin/bstshutdown"),
    OBF("/system/app/IME/IME.apk"),
    OBF("system/app/IME/IME.apk"),
    OBF("/system/app/MuMuAudio/MuMuAudio.apk"),
    OBF("/system/app/com.mumu.store/com.mumu.store.apk"),
    OBF("/system/priv-app/MuMuAudio/MuMuAudio.apk"),
    OBF("/system/priv-app/com.mumu.store_overseas/com.mumu.store_overseas.apk"),
    OBF("/system/app/KiwiIntentSink/KiwiIntentSink.apk"),
    OBF("/system/bin/droid4x"),
    OBF("/system/bin/droid4x-prop"),
    OBF("/system/bin/androVM-prop"),
    OBF("/system/bin/androVM-vbox-sf"),
    OBF("/fstab.intel"),
    OBF("/fstab.vbox86"),
    OBF("/fstab.vbox64"),
    OBF("/fstab.android_x86"),
    OBF("/fstab.android_x86_64"),
    OBF("/system/app/EmuCoreService/EmuCoreService.apk"),
    OBF("/system/app/EmuInputService/EmuInputService.apk"),
    OBF("/data/local/tmp/libLibServer.so"),
    OBF("/sbin/su"),
    OBF("/su/bin/su"),
    OBF("/data/local/su"),
    OBF("/data/local/bin/su"),
    OBF("/data/local/xbin/su"),
    OBF("/system/bin/su"),
    OBF("/system/bin/.ext/.su"),
    OBF("/system/bin/failsafe/su"),
    OBF("/system/sd/xbin/su"),
    OBF("/system/usr/we-need-root/su"),
    OBF("/system/xbin/su"),
    OBF("/cache/su"),
    OBF("/data/su"),
    OBF("/dev/su"),
    OBF("/system/etc/init/magisk"),
    OBF("/sbin/magisk"),
    OBF("/sbin/.magisk"),
    OBF("/sbin/magiskpolicy"),
    OBF("/sbin/shamiko"),
    OBF("/x8.prop"),
    OBF("/x8"),
    OBF("/data/data/com.vmos.romex"),
    OBF("/share/vpgg_phone_gpu_data.xml"),
    OBF("/share/vpgg_phone_model_data.xml"),
    OBF("/data/data/com.og.gamecenter"),
    OBF("/data/data/com.og.launcher"),
    OBF("/data/data/com.og.toolcenter"),
    OBF("/data/user/0/com.app.hider.master.lite/gaia/data/app/catch_.me_.if_.you_.can_"),
    OBF("/data/user/0/com.app.hider.master.lite/gaia/data/app/com.iavklwvofkdvxbqtstkn"),
    OBF("/data/user/0/com.app.hider.master.pro/gaia/data/app/com.iavklwvofkdvxbqtstkn"),
    OBF("/data/user/0/com.app.hider.master.pro/gaia/data/app/catch_.me_.if_.you_.can_"),
    OBF("/data/user/0/com.devadvance.rootcloak2"),
    OBF("/data/user/0/com.devadvance.rootcloak"),
    OBF("/data/user/0/biz.bokhorst.xprivacy"),
};

static bool isPathBlocked(const char *pathname) {
    for (const char *blocked : g_accessBlockList) {
        if (strstr(pathname, blocked) != nullptr) return true;
    }
    return false;
}

namespace {

int (*old_access)(const char *pathname, int mode);

int hooked_access(const char *pathname, int mode) {
    if (!pathname) {
        if (old_access) return old_access(pathname, mode);
        return access(pathname, mode);
    }
    if (isPathBlocked(pathname)) return -1;
    if (old_access) return old_access(pathname, mode);
    return access(pathname, mode);
}

}

void hack_prepare(const char *game_data_dir) {
    void *libc_handle = xdl_open(OBF("libc.so"), XDL_DEFAULT);
    if (libc_handle) {
        void *addr = xdl_sym(libc_handle, OBF("access"), nullptr);
        if (addr && addr != (void *)-1) {
            if (DobbyHook(addr, (void *)hooked_access, (void **)&old_access) == 0) {
                LOGI(OBF("Hooked access"));
            } else {
                LOGE(OBF("Failed to hook access"));
            }
        }
        xdl_close(libc_handle);
    } else {
        LOGE(OBF("Failed to open libc.so"));
    }
    std::string payload_path = std::string(game_data_dir) + OBF("/libattack.so");
    int fd = open(payload_path.c_str(), O_RDONLY);
    if (fd == -1) {
        LOGE(OBF("Unable to open payload: %s"), payload_path.c_str());
        return;
    }
    struct stat sb{};
    fstat(fd, &sb);
    size_t length = (size_t)sb.st_size;
    void *data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (data == MAP_FAILED) {
        LOGE(OBF("mmap payload failed"));
        return;
    }
    int api_level = android_get_device_api_level();
    while (!NativeBridgeLoad(game_data_dir, api_level, data, length)) {
        sleep(1);
    }
}

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
#else
REGISTER_ZYGISK_MODULE(MyModule)
#endif
