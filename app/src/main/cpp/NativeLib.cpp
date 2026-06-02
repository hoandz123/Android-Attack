#include <curl/curl.h>
#include <jni.h>
#include <string>
#include "Games.hpp"
#include "Menu.hpp"
#include <Tools/Tools.h>

#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>

#include <ActivityTracker/ActivityTracker.hpp>
#include <DexLoader/DexLoader.hpp>
#include <JNIHelper/JNIHelper.hpp>
#include <embedded_dex.hpp>
#include <ModUi.hpp>
#include <KittyMemory.h>

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    static bool s_plugin_ready = false;
    if (s_plugin_ready) return JNI_VERSION_1_6;

    if (!jni::Init(vm)) return JNI_ERR;
    JNIEnv *env = jni::Env();
    if (!env) return JNI_ERR;
    if (!dex_loader::Init(vm, embedded_dex::data, embedded_dex::size)) return JNI_ERR;
    if (!activity_tracker::Init(vm)) return JNI_ERR;
    if (!modui::Init()) return JNI_ERR;
    std::string pkg = Tools::GetPackageName();
    LOGI(OBF("package=%s"), pkg.c_str());

    if (!games::Dispatch(pkg.c_str())) appui::RegisterMenu();
    LOGI(OBF("curl %s, mod-ui ready, page=%ld"), curl_version(), (long) _SYS_PAGE_SIZE_);
    s_plugin_ready = true;
    return JNI_VERSION_1_6;
}
