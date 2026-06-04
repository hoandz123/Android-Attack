#include <curl/curl.h>
#include <atomic>
#include <jni.h>
#include <string>
#include <thread>
#include "Games.hpp"
#include "Menu.hpp"
#include <Tools/Tools.h>

#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>

#include <ActivityTracker/ActivityTracker.hpp>
#include <DexLoader/DexLoader.hpp>
#include <JNIHelper/JNIHelper.hpp>
#include <embedded_dex.hpp>
#include <ModUi.hpp>
#include <KittyMemory.h>

static bool PluginInitThread(JavaVM *vm) {
    jni::ScopedEnv env;
    if (!env.ok()) {
        LOGE(OBF("PluginInitThread: no JNIEnv"));
        return false;
    }
    jobject app = dex_loader::WaitForApplication(env.get());
    if (!app) {
        LOGE(OBF("PluginInitThread: Application not ready"));
        return false;
    }
    env->DeleteLocalRef(app);

    if (!dex_loader::Init(vm, embedded_dex::data, embedded_dex::size)) {
        LOGE(OBF("PluginInitThread: dex_loader::Init failed"));
        return false;
    }
    if (!activity_tracker::Init(vm)) {
        LOGE(OBF("PluginInitThread: activity_tracker::Init failed"));
        return false;
    }
    if (!modui::Init()) {
        LOGE(OBF("PluginInitThread: modui::Init failed"));
        return false;
    }
    std::string pkg = Tools::GetPackageName();
    LOGI(OBF("package=%s"), pkg.c_str());

    if (!games::Dispatch(pkg.c_str())) appui::RegisterMenu();
    LOGI(OBF("curl %s, mod-ui ready, page=%ld"), curl_version(), (long)_SYS_PAGE_SIZE_);
    return true;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    if (!Tools::IsMainProcess()) {
        LOGI(OBF("sub-process, skip plugin load"));
        return JNI_VERSION_1_6;
    }
    static std::atomic<bool> s_plugin_ready{false};
    static std::atomic<bool> s_init_started{false};
    if (s_plugin_ready.load()) return JNI_VERSION_1_6;

    if (!jni::Init(vm)) return JNI_ERR;

    if (!s_init_started.exchange(true)) {
        std::thread([vm]() {
            if (PluginInitThread(vm)) s_plugin_ready.store(true);
        }).detach();
    }
    return JNI_VERSION_1_6;
}
