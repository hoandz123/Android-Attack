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

bool dlopenAbsolute(const char *path, bool required) {
    if (path == nullptr || path[0] == '\0') {
        LOGE("empty library path");
        return !required;
    }

    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (handle == nullptr) {
        LOGE("dlopen failed: %s (%s)", path, dlerror());
        return !required;
    }

    LOGI("loaded %s", path);
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

} // namespace

namespace attack::loader {

bool bootstrap(JavaVM *vm, void * /*reserved*/) {
    g_vm = vm;
    LOGI("JNI_OnLoad — client entry");
    return true;
}

bool loadFromDir(const char *nativeLibraryDir, const char *libName) {
    if (nativeLibraryDir == nullptr || libName == nullptr) {
        LOGE("loadFromDir: invalid args");
        return false;
    }

    std::string fileName = libName;
    if (fileName.rfind("lib", 0) != 0) {
        fileName = "lib" + fileName;
    }
    if (fileName.size() < 3 || fileName.substr(fileName.size() - 3) != ".so") {
        fileName += ".so";
    }

    return dlopenAbsolute(joinPath(nativeLibraryDir, fileName.c_str()).c_str(), true);
}

bool loadDownloaded(const char *absolutePath) {
    return dlopenAbsolute(absolutePath, true);
}

} // namespace attack::loader