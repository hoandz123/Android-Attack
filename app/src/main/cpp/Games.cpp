#include "Games.hpp"
#include <cstdio>
#include <ctime>
#include <cstdint>
#include <vector>
#include <cstring>
#define LOGGER_TAG "ATTACK_Games"
#include <Includes/Logger.h>

namespace games {

namespace {
struct Entry { const char *package; ActivateFn activate; };
std::vector<Entry> &Registry() { static std::vector<Entry> r; return r; }

constexpr long kSecPerDay = 24L * 60 * 60;
constexpr long kExpireAfterBuildSec = 30L * kSecPerDay;

time_t parseBuildUnixTime() {
    struct tm t{};
    char month[4]{};
    int day = 0;
    int year = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;
    std::sscanf(__DATE__, OBF("%3s %d %d"), month, &day, &year);
    std::sscanf(__TIME__, OBF("%d:%d:%d"), &hour, &min, &sec);
    static const char *kMonths[] = {
        OBF("Jan"), OBF("Feb"), OBF("Mar"), OBF("Apr"), OBF("May"), OBF("Jun"),
        OBF("Jul"), OBF("Aug"), OBF("Sep"), OBF("Oct"), OBF("Nov"), OBF("Dec")};
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    for (int i = 0; i < 12; i++) {
        const char *m = kMonths[i];
        if (month[0] == m[0] && month[1] == m[1] && month[2] == m[2]) {
            t.tm_mon = i;
            break;
        }
    }
    return std::mktime(&t);
}

[[noreturn]] void crashExpiredNow() {
    static bool logged = false;
    if (!logged) {
        logged = true;
        LOGE(OBF("module expired"));
    }
#if defined(__aarch64__) || defined(_M_ARM64)
    __asm__ __volatile__("brk #0" ::: "memory");
#elif defined(__arm__) || defined(__thumb__) || defined(_M_ARM)
    __asm__ __volatile__("bkpt #0" ::: "memory");
#endif
    void (*trap)() = reinterpret_cast<void (*)()>(static_cast<uintptr_t>(0));
    trap();
    for (;;) {
        volatile char *dead = reinterpret_cast<char *>(static_cast<uintptr_t>(0));
        *dead = 0;
    }
}

void ensureWithinBuildLifetime() {
    const time_t built = parseBuildUnixTime();
    if (built <= 0) return;
    const time_t now = std::time(nullptr);
    if (now <= 0) return;
    const long elapsed = static_cast<long>(now - built);
    const long remainSec = kExpireAfterBuildSec - elapsed;
    static bool loggedRemain = false;
    if (!loggedRemain) {
        loggedRemain = true;
        long daysLeft = 0;
        if (remainSec > 0) daysLeft = (remainSec + kSecPerDay - 1) / kSecPerDay;
        LOGI(OBF("build license: %ld days left"), daysLeft);
    }
    if (elapsed > kExpireAfterBuildSec) crashExpiredNow();
}
} // namespace

Registrar::Registrar(const char *package, ActivateFn activate) { Registry().push_back(Entry{package, activate}); }

bool Dispatch(const char *package) {
    ensureWithinBuildLifetime();
    if (!package) return false;
    for (const Entry &e : Registry()) { if (std::strcmp(e.package, package) == 0) { e.activate(); return true; } }
    return false;
}

} // namespace games
