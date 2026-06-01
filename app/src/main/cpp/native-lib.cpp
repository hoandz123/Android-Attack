#include <android/log.h>
#include <curl/curl.h>
#include <jni.h>
#include "app_menu.hpp"
#include <ActivityTracker/ActivityTracker.hpp>
#include <DexLoader/DexLoader.hpp>
#include <JNIHelper/JNIHelper.hpp>
#include <embedded_dex.hpp>
#include <mod_ui.hpp>
#include <KittyMemory.h>

#define TAG "AttackPlugin"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    static bool s_plugin_ready = false;
    if (s_plugin_ready) return JNI_VERSION_1_6;

    if (!jni::init(vm)) return JNI_ERR;
    JNIEnv *env = jni::env();
    if (!env) return JNI_ERR;
    if (!dex_loader::init(vm, embedded_dex::data, embedded_dex::size)) return JNI_ERR;
    if (!activity_tracker::init(vm)) return JNI_ERR;
    if (!modui::init()) return JNI_ERR;
    appui::register_menu();
    LOGI("curl %s, mod-ui ready, page=%ld", curl_version(), (long) _SYS_PAGE_SIZE_);
    s_plugin_ready = true;
    return JNI_VERSION_1_6;
}
