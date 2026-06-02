//
// Created by TEAMHMG on 03/09/2025.
//

#pragma once

#include <stdint.h>
#include <string>
#include <obfuscate.h>
#if defined(__aarch64__)
#define HEX_NOP OBF("C0 03 5F D6")
#define HEX_RET_TRUE OBF("01 00 80 D2 C0 03 5F D6")
#define HEX_RET_FALSE OBF("00 00 80 D2 C0 03 5F D6")
#else
#define HEX_NOP "1E FF 2F E1"
#define HEX_RET_TRUE "01 00 A0 E3 1E FF 2F E1"
#define HEX_RET_FALSE "00 00 A0 E3 1E FF 2F E1"
#endif

#define RATE_LIMIT(ms)                                              \
    do {                                                            \
        static long chodoi = 0;                                     \
        if (chodoi == 0) chodoi = Tools::getSystemMilliseconds();   \
        long __rl_now = Tools::getSystemMilliseconds();             \
        if (__rl_now - chodoi < (ms))                               \
            return;                                                 \
        chodoi = 0;                                                 \
    } while (0)



#define HOOK_LIB(lib, offset, ptr, orig) Tools::Hook((void *)Tools::GetAbsoluteAddress(OBF(lib), Tools::String2Offset(OBF(offset))), (void *)ptr, (void **)&orig)


namespace Tools {
    void Hook(void* target, void* replacement, void** original = nullptr);
    uintptr_t GetBaseAddress(const char* name);
    uintptr_t GetEndAddress(const char* name);
    uintptr_t GetAbsoluteAddress(const char* libName, uintptr_t offset);
    uintptr_t String2Offset(const char *c);
    uintptr_t FindSymbol(const char* module, const char* symbol);
    std::string GetPackageName();
    std::string getApkPath();
    long long getSystemMilliseconds();
    void Sleep(int ms);
};


