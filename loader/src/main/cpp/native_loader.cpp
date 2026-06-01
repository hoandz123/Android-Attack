#include "native_loader.hpp"

#include <android/log.h>
#include <dlfcn.h>

#include <cstring>
#include <string>
#include <vector>

#define LOG_TAG "AttackLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

JavaVM *g_vm = nullptr;

struct LibSpec {
    const char *fileName;
    bool required;
};

// Built-in libs shipped inside APK lib/<abi>/.
const std::vector<LibSpec> &builtinLibs() {
    static const std::vector<LibSpec> libs = {
        {"libattack.so", true},
    };
    return libs;
}

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

bool dlopenByName(const char *libFileName, bool required) {
    if (libFileName == nullptr || libFileName[0] == '\0') {
        LOGE("empty library name");
        return !required;
    }

    void *handle = dlopen(libFileName, RTLD_NOW | RTLD_GLOBAL);
    if (handle != nullptr) {
        LOGI("loaded %s", libFileName);
        return true;
    }

    LOGE("dlopen by name failed: %s (%s)", libFileName, dlerror());
    return !required;
}

std::string joinPath(const char *dir, const char *fileName) {
    std::string path(dir);
    if (!path.empty() && path.back() != '/') {
        path += '/';
    }
    path += fileName;
    return path;
}

bool loadBuiltinLibs() {
    bool ok = true;
    for (const LibSpec &spec : builtinLibs()) {
        if (!dlopenByName(spec.fileName, spec.required)) {
            ok = false;
        }
    }
    return ok;
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

    if (!loadBuiltinLibs()) {
        LOGE("built-in native bootstrap failed");
        return false;
    }

    LOGI("bootstrap complete");
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