#pragma once

#include <android/log.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define HIDETAG "AttackHide"
#define HIDEL(...) __android_log_print(ANDROID_LOG_INFO, HIDETAG, __VA_ARGS__)
#define HIDEE(...) __android_log_print(ANDROID_LOG_ERROR, HIDETAG, __VA_ARGS__)

namespace HideTools {

#if defined(__LP64__)
using H_Elf_Ehdr = Elf64_Ehdr;
using H_Elf_Phdr = Elf64_Phdr;
using H_Elf_Dyn = Elf64_Dyn;
using H_Elf_Addr = Elf64_Addr;
using H_Elf_Word = Elf64_Word;
using H_Elf_Sword = Elf64_Sword;
#else
using H_Elf_Ehdr = Elf32_Ehdr;
using H_Elf_Phdr = Elf32_Phdr;
using H_Elf_Dyn = Elf32_Dyn;
using H_Elf_Addr = Elf32_Addr;
using H_Elf_Word = Elf32_Word;
using H_Elf_Sword = Elf32_Sword;
#endif

struct LoadRange { uintptr_t base; uintptr_t lo; uintptr_t hi; bool ok; };

inline size_t pageSize() { static size_t p = (size_t)sysconf(_SC_PAGESIZE); return p ? p : 4096; }

inline uintptr_t mapAddr(uintptr_t base, H_Elf_Addr v) {
    if (!v) return 0;
    if ((uintptr_t)v >= base) return (uintptr_t)v;
    return base + (uintptr_t)v;
}

inline bool wipeRegion(void *addr, size_t len, int prot) {
    if (!addr || !len) return true;
    size_t ps = pageSize();
    uintptr_t start = (uintptr_t)addr & ~(ps - 1);
    uintptr_t end = ((uintptr_t)addr + len + ps - 1) & ~(ps - 1);
    if (mprotect((void *)start, end - start, PROT_READ | PROT_WRITE) != 0) { HIDEE("mprotect rw %p: %s", addr, strerror(errno)); return false; }
    memset(addr, 0, len);
    if (mprotect((void *)start, end - start, prot ? prot : PROT_NONE) != 0) { HIDEE("mprotect restore %p: %s", addr, strerror(errno)); return false; }
    return true;
}

inline int rangePhdr(struct dl_phdr_info *info, size_t, void *data) {
    LoadRange *r = (LoadRange *)data;
    if ((uintptr_t)info->dlpi_addr != r->base) return 0;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const H_Elf_Phdr *p = (const H_Elf_Phdr *)&info->dlpi_phdr[i];
        if (p->p_type != PT_LOAD) continue;
        uintptr_t s = (uintptr_t)info->dlpi_addr + p->p_vaddr;
        uintptr_t e = s + p->p_memsz;
        if (!r->ok || s < r->lo) r->lo = s;
        if (!r->ok || e > r->hi) r->hi = e;
        r->ok = true;
    }
    return 0;
}

inline bool loadRange(uintptr_t base, LoadRange *out) {
    out->base = base;
    out->lo = base;
    out->hi = base;
    out->ok = false;
    dl_iterate_phdr(rangePhdr, out);
    return out->ok;
}

inline void wipeGuardPages(const LoadRange *r) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start = 0, end = 0;
        char perms[8] = {};
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) != 3) continue;
        if (strcmp(perms, "---p") != 0) continue;
        if (end <= r->lo || start >= r->hi) continue;
        wipeRegion((void *)start, (size_t)(end - start), PROT_NONE);
        HIDEL("guard %lx-%lx", (unsigned long)start, (unsigned long)end);
    }
    fclose(fp);
}

inline size_t hashTableSize(uintptr_t hash) {
    if (!hash) return 0;
    const H_Elf_Word *h = (const H_Elf_Word *)hash;
    return (size_t)(2 + h[0] + h[1]) * sizeof(H_Elf_Word);
}

inline size_t gnuHashSize(uintptr_t gnu, uintptr_t symtab, size_t syment, uintptr_t strtab) {
    if (!gnu) return 0;
    if (strtab > gnu) return (size_t)(strtab - gnu);
    if (symtab > gnu) return (size_t)(symtab - gnu) + syment * 64;
    return 512;
}

inline bool inLoadRange(const LoadRange *r, uintptr_t addr, size_t len) {
    if (!r || !r->ok || !len) return false;
    return addr >= r->lo && addr + len <= r->hi;
}

inline bool wipeDynamic(uintptr_t base, const H_Elf_Phdr *dynPhdr, const LoadRange *range) {
    uintptr_t dynAddr = mapAddr(base, (H_Elf_Addr)dynPhdr->p_vaddr);
    H_Elf_Addr hash = 0, gnu = 0, sym = 0, str = 0;
    size_t syment = sizeof(Elf64_Sym);
#if !defined(__LP64__)
    syment = sizeof(Elf32_Sym);
#endif
    size_t strsz = 0;
    for (H_Elf_Dyn *d = (H_Elf_Dyn *)dynAddr; d->d_tag != DT_NULL; d++) {
        if (d->d_tag == DT_HASH) hash = d->d_un.d_ptr;
        else if (d->d_tag == DT_GNU_HASH) gnu = d->d_un.d_ptr;
        else if (d->d_tag == DT_STRTAB) str = d->d_un.d_ptr;
        else if (d->d_tag == DT_STRSZ) strsz = (size_t)d->d_un.d_val;
        else if (d->d_tag == DT_SYMTAB) sym = d->d_un.d_ptr;
        else if (d->d_tag == DT_SYMENT) syment = (size_t)d->d_un.d_val;
    }
    uintptr_t hashAddr = mapAddr(base, hash), gnuAddr = mapAddr(base, gnu), symAddr = mapAddr(base, sym), strAddr = mapAddr(base, str);
    size_t symCount = hashAddr ? ((const H_Elf_Word *)hashAddr)[1] : 0;
    if (symCount > 8192) symCount = 0;
    if (hashAddr) { size_t n = hashTableSize(hashAddr); if (inLoadRange(range, hashAddr, n)) wipeRegion((void *)hashAddr, n, PROT_READ); }
    if (gnuAddr) { size_t n = gnuHashSize(gnuAddr, symAddr, syment, strAddr); if (n > 4096) n = 4096; if (inLoadRange(range, gnuAddr, n)) wipeRegion((void *)gnuAddr, n, PROT_READ); }
    if (symAddr && symCount) { size_t n = symCount * syment; if (inLoadRange(range, symAddr, n)) wipeRegion((void *)symAddr, n, PROT_READ); }
    if (strAddr && strsz && strsz < 1024 * 1024) { if (inLoadRange(range, strAddr, strsz)) wipeRegion((void *)strAddr, strsz, PROT_READ); }
    return true;
}

inline bool mangleElf(uintptr_t base) {
    const H_Elf_Ehdr *eh = (const H_Elf_Ehdr *)base;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) { HIDEE("bad elf magic %p", (void *)base); return false; }
    const H_Elf_Phdr *phdr = (const H_Elf_Phdr *)(base + eh->e_phoff);
    LoadRange range{};
    loadRange(base, &range);
    for (int i = 0; i < eh->e_phnum; i++) {
        if (phdr[i].p_type == PT_NOTE && phdr[i].p_memsz) wipeRegion((void *)mapAddr(base, (H_Elf_Addr)phdr[i].p_vaddr), (size_t)phdr[i].p_memsz, PROT_READ);
        if (phdr[i].p_type == PT_DYNAMIC) wipeDynamic(base, &phdr[i], &range);
    }
    size_t phdrBytes = (size_t)eh->e_phnum * eh->e_phentsize;
    wipeRegion((void *)base, sizeof(H_Elf_Ehdr), PROT_READ | PROT_EXEC);
    wipeRegion((void *)(base + eh->e_phoff), phdrBytes, PROT_READ | PROT_EXEC);
    HIDEL("ehdr+phdr %p", (void *)base);
    if (range.ok) wipeGuardPages(&range);
    HIDEL("mangle done base=%p", (void *)base);
    return true;
}

inline int touchPhdr(struct dl_phdr_info *info, size_t, void *data) {
    uintptr_t base = *(uintptr_t *)data;
    if ((uintptr_t)info->dlpi_addr != base) return 0;
    size_t ps = pageSize();
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const H_Elf_Phdr *p = (const H_Elf_Phdr *)&info->dlpi_phdr[i];
        if (p->p_type != PT_LOAD) continue;
        uintptr_t s = (uintptr_t)info->dlpi_addr + p->p_vaddr;
        uintptr_t e = s + p->p_memsz;
        for (uintptr_t a = s; a < e; a += ps) { volatile uint8_t b = *(uint8_t *)a; (void)b; }
    }
    return 0;
}

inline void touchLoadPages(uintptr_t base) { dl_iterate_phdr(touchPhdr, &base); }

inline bool zeroMemfdBacking(int fd, size_t size) {
    if (fd < 0 || !size) return false;
    uint8_t buf[4096]{};
    if (lseek(fd, 0, SEEK_SET) < 0) { HIDEE("lseek memfd: %s", strerror(errno)); return false; }
    for (size_t off = 0; off < size; ) {
        size_t n = size - off;
        if (n > sizeof(buf)) n = sizeof(buf);
        if (write(fd, buf, n) != (ssize_t)n) { HIDEE("zero memfd: %s", strerror(errno)); return false; }
        off += n;
    }
    HIDEL("memfd backing zeroed (%zu)", size);
    return true;
}

inline bool manglePlugin(void *handle, int memfdFd, size_t memfdSize) {
    if (!handle) return false;
    void *sym = dlsym(handle, "JNI_OnLoad");
    if (!sym) { HIDEE("dlsym JNI_OnLoad: %s", dlerror()); return false; }
    Dl_info info{};
    if (!dladdr(sym, &info) || !info.dli_fbase) { HIDEE("dladdr failed"); return false; }
    uintptr_t base = (uintptr_t)info.dli_fbase;
    touchLoadPages(base);
    if (!mangleElf(base)) return false;
    if (memfdFd >= 0) { touchLoadPages(base); return zeroMemfdBacking(memfdFd, memfdSize); }
    return true;
}

inline bool manglePlugin(void *handle) { return manglePlugin(handle, -1, 0); }

inline void zeroBuffer(void *data, size_t size) {
    if (data && size) memset(data, 0, size);
}

}
