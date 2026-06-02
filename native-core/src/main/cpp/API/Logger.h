#pragma once

#include <android/log.h>

#define API_TAG "api_sdk"

#define SDKLOGI(...) __android_log_print(ANDROID_LOG_INFO,  API_TAG, __VA_ARGS__)
#define SDKLOGW(...) __android_log_print(ANDROID_LOG_WARN,  API_TAG, __VA_ARGS__)
#define SDKLOGE(...) __android_log_print(ANDROID_LOG_ERROR, API_TAG, __VA_ARGS__)
#define SDKLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, API_TAG, __VA_ARGS__)

#ifndef JLOG_HEX
#define JLOG_HEX(x) static_cast<unsigned long long>(x)
#endif
