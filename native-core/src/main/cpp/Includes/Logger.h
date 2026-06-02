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

#ifndef LOGGER_TAG
#define LOGGER_TAG "ATTACK_ModMenu"
#endif

#ifndef LOG_TAG
#define LOG_TAG OBF(LOGGER_TAG)
#endif

#if defined(NDEBUG) && !defined(FORCE_LOG)
#define LOGD(...) ((void)0)
#define LOGE(...) ((void)0)
#define LOGI(...) ((void)0)
#define LOGW(...) ((void)0)
#else
#define LOGD(...) do { _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-security\"") (void)__android_log_print(oDEBUG, LOG_TAG, __VA_ARGS__); _Pragma("clang diagnostic pop") } while(0)
#define LOGE(...) do { _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-security\"") (void)__android_log_print(oERROR, LOG_TAG, __VA_ARGS__); _Pragma("clang diagnostic pop") } while(0)
#define LOGI(...) do { _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-security\"") (void)__android_log_print(oINFO, LOG_TAG, __VA_ARGS__); _Pragma("clang diagnostic pop") } while(0)
#define LOGW(...) do { _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wformat-security\"") (void)__android_log_print(oWARN, LOG_TAG, __VA_ARGS__); _Pragma("clang diagnostic pop") } while(0)
#endif

#endif /* Logger_h */
