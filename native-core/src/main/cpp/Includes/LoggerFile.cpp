#include "LoggerFile.h"
#include <Includes/obfuscate.h>
#include <android/log.h>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>
#include <unistd.h>

namespace {

std::mutex gLogMu;
std::string gCustomPath;
bool gEnabled = true;
bool gHeaderWritten = false;
FILE *gFile = nullptr;

std::string ReadCmdlinePackage() {
    char buf[256]{};
    FILE *f = std::fopen(OBF("/proc/self/cmdline"), OBF("rb"));
    if (!f) return {};
    const size_t n = std::fread(buf, 1, sizeof(buf) - 1, f);
    std::fclose(f);
    std::string pkg(buf, n);
    const size_t z = pkg.find('\0');
    if (z != std::string::npos) pkg.resize(z);
    return pkg;
}

std::string DefaultLogPath() {
    std::string pkg = ReadCmdlinePackage();
    if (pkg.empty()) pkg = OBF("unknown");
    return std::string(OBF("/storage/emulated/0/Android/data/")) + pkg + OBF("/attack.log");
}

const char *LevelChar(int level) {
    if (level == ANDROID_LOG_ERROR) return OBF("E");
    if (level == ANDROID_LOG_WARN) return OBF("W");
    if (level == ANDROID_LOG_DEBUG) return OBF("D");
    return OBF("I");
}

bool EnsureFileOpen() {
    if (gFile) return true;
    const std::string path = gCustomPath.empty() ? DefaultLogPath() : gCustomPath;
    const std::string dir = path.substr(0, path.rfind('/'));
    if (!dir.empty()) std::filesystem::create_directories(dir);
    gFile = std::fopen(path.c_str(), OBF("a"));
    if (gFile) {
        static bool sAnnounced = false;
        if (!sAnnounced) {
            sAnnounced = true;
            __android_log_print(ANDROID_LOG_INFO, OBF("ATTACK_LogFile"), OBF("ghi log file: %s"), path.c_str());
        }
    }
    return gFile != nullptr;
}

void WriteHeaderIfNeeded() {
    if (gHeaderWritten || !gFile) return;
    const std::string path = gCustomPath.empty() ? DefaultLogPath() : gCustomPath;
    std::fprintf(gFile, OBF("--- attack.log pid=%d path=%s ---\n"), getpid(), path.c_str());
    std::fflush(gFile);
    gHeaderWritten = true;
}

void WriteLine(int level, const char *tag, const char *msg) {
    if (!gEnabled || !msg || !tag) return;
    std::lock_guard<std::mutex> lock(gLogMu);
    if (!EnsureFileOpen()) return;
    WriteHeaderIfNeeded();
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto sec = time_point_cast<seconds>(now);
    const auto ms = duration_cast<milliseconds>(now - sec).count();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char ts[32]{};
    std::snprintf(ts, sizeof(ts), OBF("%04d-%02d-%02d %02d:%02d:%02d.%03d"), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, static_cast<int>(ms));
    std::fprintf(gFile, OBF("%s %s %s %s\n"), ts, LevelChar(level), tag, msg);
    std::fflush(gFile);
}

} // namespace

extern "C" void LoggerFileSetEnabled(int enabled) {
    std::lock_guard<std::mutex> lock(gLogMu);
    gEnabled = enabled != 0;
}

extern "C" void LoggerFileSetPath(const char *path) {
    std::lock_guard<std::mutex> lock(gLogMu);
    if (gFile) {
        std::fclose(gFile);
        gFile = nullptr;
        gHeaderWritten = false;
    }
    gCustomPath = path ? path : "";
}

extern "C" void LoggerFileAppend(int level, const char *tag, const char *fmt, ...) {
    if (!gEnabled || !fmt || !tag) return;
    char buf[4096]{};
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    WriteLine(level, tag, buf);
}
