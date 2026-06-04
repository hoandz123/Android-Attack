#pragma once

#include <Includes/Logger.h>
#include <kittymemory/KittyMemory.h>
#include <kittymemory/MemoryPatch.h>
#include <unistd.h>
#include <cstdint>

namespace lienquan {
namespace anogs_bypass {

namespace {

constexpr const char kLibAnogs[] = "libanogs.so";

constexpr const char kHexRet[] = "C0 03 5F D6";
constexpr const char kHexNop[] = "1F 20 03 D5";
constexpr const char kHexMovX0Zero[] = "00 00 80 D2";

struct AnogsPatch {
    uintptr_t offset;
    const char *hex;
};

static const AnogsPatch kAnogsPatches[] = {
    {0x409330, kHexRet},
    {0x40977C, kHexRet},
    {0x49F6C0, kHexRet},
    {0x4DE048, kHexNop},
    {0x4FED38, kHexRet},
    {0x51CA08, kHexRet},
    {0x54A550, kHexRet},
    {0x530E70, kHexRet},
    {0x54A6C0, kHexMovX0Zero},
    {0x54A6C4, kHexRet},
    {0x54A800, kHexRet},
};

bool wait_for_library(int maxSeconds = 120) {
    for (int i = 0; i < maxSeconds; ++i) {
        if (KittyMemory::getAbsoluteAddress(kLibAnogs, 0) != 0) return true;
        if (i == 0) LOGI(OBF("[LienQuan/anogs] waiting for %s..."), kLibAnogs);
        sleep(1);
    }
    return false;
}

} // namespace

inline void apply_anogs_patches() {
    if (!wait_for_library()) {
        LOGE(OBF("[LienQuan/anogs] %s not found after 120s"), kLibAnogs);
        return;
    }

    int ok = 0, fail = 0;
    for (const auto &p : kAnogsPatches) {
        MemoryPatch patch = MemoryPatch::createWithHex(kLibAnogs, p.offset, p.hex);
        if (patch.isValid() && patch.Modify())
            ++ok;
        else
            ++fail;
    }
    LOGI(OBF("[LienQuan/anogs] MemoryPatch: %d ok, %d fail"), ok, fail);
}

inline void install() { apply_anogs_patches(); }

} // namespace anogs_bypass
} // namespace lienquan
