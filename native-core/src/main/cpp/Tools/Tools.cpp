//
// Created by TEAMHMG on 03/09/2025.
//

#include <bits/sysconf.h>
#include <asm-generic/mman-common.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <vector>
#include "Tools.h"
#include "Logger.h"
#if defined(__aarch64__)
#include "And64Hook/And64Hook.hpp"
#endif
#include "xdl/xdl.h"
#include "dobby/dobby.h"


void Tools::Hook(void *target, void *replacement, void **original) {
    LOGI("Tools::Hook target: %p, replacement: %p, original: %p", target, replacement, original);
    if (!target || !replacement) {
        LOGE("Tools::Hook NUll pointer");
        return;
    }
    unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    if (mprotect((void *) ((uintptr_t) target - ((uintptr_t) target % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0) {
#if defined(__aarch64__)
        A64Hook(target, replacement, original);
#else
        DobbyHook(target, replacement, original);
#endif
    }
}
uintptr_t Tools::GetBaseAddress(const char *name) {
    FILE *f = fopen(OBF("/proc/self/maps"), "r");
    if (!f) return 0;
    char line[512];
    uintptr_t base = 0;
    bool matchAll = (name == nullptr);
    while (fgets(line, sizeof(line), f)) {
        if (!matchAll && !strstr(line, name)) continue;
        char* endptr = nullptr;
        errno = 0;
        unsigned long long start = std::strtoull(line, &endptr, 16);
        if (endptr == line || errno == ERANGE) continue;
        if (base == 0 || (uintptr_t)start < base) base = (uintptr_t)start;
    }

    fclose(f);
    return base;
}

uintptr_t Tools::GetEndAddress(const char *name) {
    FILE *f = fopen(OBF("/proc/self/maps"), "r");
    if (!f) return 0;
    char line[512];
    uintptr_t endAddr = 0;
    bool matchAll = (name == nullptr);
    while (fgets(line, sizeof(line), f)) {
        if (!matchAll && !strstr(line, name)) continue;
        char* endptr = nullptr;
        errno = 0;
        unsigned long long start = std::strtoull(line, &endptr, 16);
        if (endptr == line || errno == ERANGE) continue;
        unsigned long long end_val = start;
        if (*endptr == '-') {
            char* nextptr = nullptr;
            errno = 0;
            end_val = std::strtoull(endptr + 1, &nextptr, 16);
            if (nextptr == endptr + 1 || errno == ERANGE) end_val = start;
        }
        if ((uintptr_t)end_val > endAddr) endAddr = (uintptr_t)end_val;
    }
    fclose(f);
    return endAddr;
}
uintptr_t Tools::GetAbsoluteAddress(const char*libName, uintptr_t offset) {
    uintptr_t libBase = GetBaseAddress(libName);
    if (!libBase) return 0;
    return (reinterpret_cast<uintptr_t>(libBase + offset));
}
uintptr_t Tools::String2Offset(const char* c) {
    if (c == nullptr || *c == '\0') {
        return 0;
    }

    int base = 16;
    char* endptr = nullptr;
    errno = 0;

    static_assert(sizeof(uintptr_t) == sizeof(unsigned long) ||
            sizeof(uintptr_t) == sizeof(unsigned long long),
            "Please add string to handle conversion for this architecture.");

    uintptr_t result = 0;

    if (sizeof(uintptr_t) == sizeof(unsigned long)) {
        result = strtoul(c, &endptr, base);
    } else {
        result = strtoull(c, &endptr, base);
    }
    if (endptr == c || errno == ERANGE) {
        return 0;
    }

    return result;
}

uintptr_t Tools::FindSymbol(const char *module, const char *symbol) {
    return reinterpret_cast<uintptr_t>(DobbySymbolResolver(module, symbol));
}
std::string Tools::GetPackageName() {
    static std::string packageName;
    if (!packageName.empty()) {
        return packageName;
    }
    char buf[256] = {};
    if (FILE* fp = std::fopen(OBF("/proc/self/cmdline"), "r")) {
        std::fread(buf, 1, sizeof(buf) - 1, fp);
        std::fclose(fp);
    }
    std::string fullName(buf);
    size_t pos = fullName.find(':');
    if (pos != std::string::npos) {
        fullName = fullName.substr(0, pos);
    }
    packageName = fullName;
    return fullName;
}

bool Tools::IsMainProcess() {
    char buf[256]{};
    if (FILE *fp = std::fopen(OBF("/proc/self/cmdline"), "r")) {
        std::fread(buf, 1, sizeof(buf) - 1, fp);
        std::fclose(fp);
    }
    for (size_t i = 0; i < sizeof(buf) && buf[i]; ++i) {
        if (buf[i] == ':') return false;
    }
    return true;
}

static bool checkApkPath(const std::string& packageName, const std::string& apkPath) {
    if (apkPath.empty() || apkPath[0] != '/' || apkPath.find(".apk") == std::string::npos) {
        return false;
    }
    
    if (apkPath.length() < 4 || apkPath.substr(apkPath.length() - 4) != ".apk") {
        return false;
    }
    
    std::string pathWithoutSlash = apkPath.substr(1);
    std::vector<std::string> parts;
    std::istringstream iss(pathWithoutSlash);
    std::string part;
    int count = 0;
    
    while (std::getline(iss, part, '/') && count < 6) {
        parts.push_back(part);
        count++;
    }
    
    int length = parts.size();
    
    if (length == 4 || length == 5) {
        if (parts[0] == OBFS("data") && parts[1] == OBFS("app") && parts[length - 1] == OBFS("base.apk")) {
            return parts[length - 2].find(packageName) == 0;
        }
        if (parts[0] == OBFS("mnt") && parts[1] == OBFS("asec") && parts[length - 1] == OBFS("pkg.apk")) {
            return parts[length - 2].find(packageName) == 0;
        }
    } else if (length == 3) {
        if (parts[0] == "data" && parts[1] == "app") {
            return parts[2].find(packageName) == 0;
        }
    } else if (length == 6) {
        if (parts[0] == OBFS("mnt") && parts[1] == OBFS("expand") && parts[3] == OBFS("app") && parts[5] == OBFS("base.apk")) {
            const std::string& part4 = parts[4];
            if (part4.length() >= packageName.length()) {
                return part4.substr(part4.length() - packageName.length()) == packageName;
            }
        }
    }
    
    return false;
}

std::string Tools::getApkPath() {
    std::string packageName = GetPackageName();
    if (packageName.empty()) {
        LOGE("getApkPath: packageName empty");
        return "";
    }
    
    FILE* f = fopen(OBF("/proc/self/maps"), "r");
    if (!f) {
        LOGE("getApkPath: failed to open /proc/self/maps");
        return "";
    }
    
    char line[1024];
    std::string apkPath;
    
    while (fgets(line, sizeof(line), f)) {
        std::string lineStr(line);
        // LOGD("getApkPath: maps line: %s", lineStr.c_str());
        
        size_t lastSpace = lineStr.find_last_of(" \t");
        if (lastSpace == std::string::npos) {
            continue;
        }
        
        std::string path = lineStr.substr(lastSpace + 1);
        while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) {
            path.pop_back();
        }
        // LOGD("getApkPath: candidate path: %s", path.c_str());
        if (checkApkPath(packageName, path)) {
            LOGI("getApkPath: matched apk path: %s", path.c_str());
            apkPath = path;
            break;
        }
    }
    
    fclose(f);
    
    if (apkPath.empty()) {
        LOGE("getApkPath: apkPath empty after scan");
        return "";
    }
    
    LOGI("getApkPath: final apkPath: %s", apkPath.c_str());
    return apkPath;
}
long long Tools::getSystemMilliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Tools::Sleep(int ms) {
    struct timespec req = { ms, 0 };
    while (nanosleep(&req, &req) == -1) {
    }
}
