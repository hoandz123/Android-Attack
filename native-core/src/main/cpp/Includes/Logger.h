//
//  Logger.h
//
//  Created by MJ (Ruit) on 1/1/19.
//

#ifndef Logger_h
#define Logger_h

#include <jni.h>
#include <android/log.h>
#include <Includes/obfuscate.h>

enum LogType {
    oDEBUG = 3,
    oERROR = 6,
    oINFO  = 4,
    oWARN  = 5
};

#define TAG OBFUSCATE("Mod_Menu")

#ifndef LOG_TAG
#define LOG_TAG TAG
#endif

#define LOGD(...) ((void)__android_log_print(oDEBUG, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(oERROR, LOG_TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(oINFO,  LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(oWARN,  LOG_TAG, __VA_ARGS__))

#endif /* Logger_h */
