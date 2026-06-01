#include <android/log.h>
#include <curl/curl.h>
#include <dobby.h>
#include <jni.h>
#include "app_menu.hpp"
#include <mod_ui.hpp>
#include <link.h>
#include <KittyMemory.h>
#include <stdio.h>
#include <string.h>
#include <string>

#define TAG "AttackPlugin"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

static jstring stringFromNative(JNIEnv *env, jclass clazz) { return env->NewStringUTF("Hello from C++ (NDK 28)"); }

static int soPhdr(struct dl_phdr_info *info, size_t, void *data) {
    std::string *out = (std::string *) data;
    const char *name = info->dlpi_name && info->dlpi_name[0] ? info->dlpi_name : "(main)";
    *out += "  ";
    *out += name;
    *out += '\n';
    return 0;
}

static void appendMaps(std::string *out) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, "r-xp")) continue;
        char *path = strrchr(line, ' ');
        if (!path) continue;
        path++;
        size_t n = strlen(path);
        if (n && path[n - 1] == '\n') path[n - 1] = 0;
        if (!path[0]) continue;
        if (path[0] == '[' || strstr(path, ".so") || strstr(path, "memfd") || strstr(path, "/proc/")) {
            *out += "  ";
            *out += path;
            *out += '\n';
        }
    }
    fclose(f);
}

static jstring listLoadedSo(JNIEnv *env, jclass clazz) {
    std::string r = "[Java System.loadLibrary]\n  loader\n\n[dl_iterate_phdr]\n";
    dl_iterate_phdr(soPhdr, &r);
    r += "\n[/proc/self/maps r-xp]\n";
    appendMaps(&r);
    return env->NewStringUTF(r.c_str());
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    modui::init();
    appui::register_menu();
    LOGI("curl %s, mod-ui ready, page=%ld", curl_version(), (long) _SYS_PAGE_SIZE_);
    JNINativeMethod m[] = {{"stringFromNative", "()Ljava/lang/String;", (void *) stringFromNative},
                           {"listLoadedSo",     "()Ljava/lang/String;", (void *) listLoadedSo}};
    jclass c = env->FindClass("com/android/attack/MainActivity");
    if (!c || env->RegisterNatives(c, m, 2) != JNI_OK) return JNI_ERR;
    return JNI_VERSION_1_6;
}
