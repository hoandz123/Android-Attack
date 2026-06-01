#include <android/log.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <jni.h>
#include <linux/memfd.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string>
#include <vector>

#define TAG "AttackLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static const char *kPlugins[] = {"libattack.so"};

static int memfd(const char *name) {
#if __ANDROID_API__ >= 29
    return memfd_create(name, MFD_CLOEXEC);
#else
    return (int)syscall(__NR_memfd_create, name, MFD_CLOEXEC);
#endif
}

static bool readFile(const char *path, std::vector<uint8_t> &buf) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return false; }
    rewind(f);
    buf.resize((size_t)sz);
    bool ok = fread(buf.data(), 1, (size_t)sz, f) == (size_t)sz;
    fclose(f);
    return ok;
}

static bool loadPluginMemfd(JavaVM *vm, void *reserved, const void *data, size_t size) {
    if (!vm || !data || !size) return false;
    int fd = memfd("");
    if (fd < 0) { LOGE("memfd: %s", strerror(errno)); return false; }
    if (write(fd, data, size) != (ssize_t)size) { close(fd); LOGE("memfd write: %s", strerror(errno)); return false; }
    char fdPath[32];
    snprintf(fdPath, sizeof(fdPath), "/proc/self/fd/%d", fd);
    void *handle = dlopen(fdPath, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) { close(fd); LOGE("dlopen memfd: %s", dlerror()); return false; }
    auto onLoad = (jint (*)(JavaVM *, void *))dlsym(handle, "JNI_OnLoad");
    if (!onLoad) { close(fd); LOGE("dlsym JNI_OnLoad: %s", dlerror()); return false; }
    if (onLoad(vm, reserved) != JNI_VERSION_1_6) { close(fd); LOGE("JNI_OnLoad failed"); return false; }
    close(fd);
    LOGI("loaded plugin (%zu bytes)", size);
    return true;
}

static bool loadPluginMemfd(JavaVM *vm, void *reserved, std::vector<uint8_t> &buf) {
    if (!vm || buf.empty()) return false;
    bool ok = loadPluginMemfd(vm, reserved, buf.data(), buf.size());
    memset(buf.data(), 0, buf.size());
    buf.clear();
    buf.shrink_to_fit();
    return ok;
}

static bool loadPlugins(JavaVM *vm, void *reserved, const std::string &dir) {
    for (const char *name : kPlugins) {
        std::string path = dir.empty() ? name : dir + "/" + name;
        std::vector<uint8_t> buf;
        if (!readFile(path.c_str(), buf)) { LOGE("read plugin failed"); return false; }
        if (!loadPluginMemfd(vm, reserved, buf)) return false;
    }
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
    nativeLibDir(env, dir);
    return loadPlugins(vm, reserved, dir) ? JNI_VERSION_1_6 : JNI_ERR;
}
