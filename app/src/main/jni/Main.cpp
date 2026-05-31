#include <thread>
#include <jni.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Core/Bootstrap.h"
#include "UI/Overlay.h"

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void) reserved;
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    bootstrap::initVm(vm);
    std::thread([]() {
        LOGI(OBFUSCATE("hack_thread started"));
        if (bootstrap::run()) {
            LOGI(OBFUSCATE("bootstrap ready"));
            if (overlay::start()) LOGI(OBFUSCATE("overlay started"));
            else LOGE(OBFUSCATE("overlay failed"));
        } else {
            LOGE(OBFUSCATE("bootstrap failed"));
        }
    }).detach();
    LOGI(OBFUSCATE("JNI_OnLoad: hack_thread spawned"));
    return JNI_VERSION_1_6;
}
