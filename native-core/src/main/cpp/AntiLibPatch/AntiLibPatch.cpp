#include "AntiLibPatch.h"
#include "Logger.h"
#include "obfuscate.h"
#include "constants.h"
#include "ElfImg.h"

#include <vector>
#include <map>

#include <elf.h>
#include <fcntl.h>
#include <dirent.h>
#include <link.h>
#include <dlfcn.h>
#include <thread>


_forceinline static uint32_t crc32(const uint8_t *data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}
const char *targetLibName;
AntiLibPatch::AntiLibPatch(void (*callback)(const char *libPath, uint32_t old_checksum, uint32_t new_checksum)
) :
onLibTampered(callback) {
        LOGI("AntiLibPatch::AntiLibPatch");
        Dl_info info;
        if (dladdr((void*)&crc32, &info) && info.dli_fname) {
            targetLibName = strrchr(info.dli_fname, '/');
            targetLibName = targetLibName ? targetLibName + 1 : info.dli_fname;
            LOGI("Tên thư viện: %s", targetLibName);
        } else {
            LOGI("Không tìm được tên thư viện!");
        }
        auto callback_dl =[](struct dl_phdr_info *info, size_t size, void *data) {
            auto self = static_cast<AntiLibPatch *>(data);
            if (!info->dlpi_name || !strlen(info->dlpi_name)) return 0;

            const char *lib_name = strrchr(info->dlpi_name, '/');
            lib_name = lib_name ? lib_name + 1 : info->dlpi_name;

            if (strcmp(lib_name, targetLibName) != 0) {
                return 0;
            }

            LOGI("base %llx, libName %s", info->dlpi_addr, info->dlpi_name);

            for (int i = 0; i < info->dlpi_phnum; ++i) {
                const ElfW(Phdr)
                *phdr = &info->dlpi_phdr[i];
                if (phdr->p_type == PT_LOAD && (phdr->p_flags & PF_X) && (phdr->p_flags & PF_R)) {
                    ElfW(Addr)
                    start = phdr->p_vaddr;
                    ElfW(Addr)
                    end = start + phdr->p_memsz;

                    auto regionAddress = reinterpret_cast<const uint8_t *>(info->dlpi_addr + start);
                    auto regionSize = end - start;
                    uint32_t checksum = crc32(regionAddress, regionSize);
                    if (checksum != 0) {
                        self->regions.emplace_back(info->dlpi_name, info->dlpi_addr, std::make_pair(start, end), checksum);
                    }
                }
            }
            return 0;
        };

        dl_iterate_phdr(callback_dl, this);
}

const char *AntiLibPatch::getName() {
    return OBF("Lib. Patch & Hook Detection");
}

eSeverity AntiLibPatch::getSeverity() {
    return HIGH;
}

bool AntiLibPatch::execute() {
//    LOGI("AntiLibPatch::execute");
    for (const auto &region: regions) {
        auto regionSize = region.chunkData.second - region.chunkData.first;
        auto *regionAddress = reinterpret_cast<uint8_t *>(region.baseAddress + region.chunkData.first);
        uint32_t currentChecksum = crc32(regionAddress, regionSize);
        if (currentChecksum != region.initialChecksum) {
            if (this->onLibTampered) {
                this->onLibTampered(region.libPath.c_str(), region.initialChecksum, currentChecksum);
            }

            return true;
        }
    }
    return false;
}

