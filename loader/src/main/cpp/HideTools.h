#pragma once

#include <android/log.h>
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
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

inline bool wipeDynamic(uintptr_t base, const H_Elf_Phdr *dynPhdr) {
    uintptr_t dynAddr = base + dynPhdr->p_vaddr;
    H_Elf_Addr hash = 0, gnu = 0, sym = 0, str = 0;
    size_t syment = sizeof(Elf64_Sym);
#if !defined(__LP64__)
    syment = sizeof(Elf32_Sym);
#endif
    size_t strsz = 0;
    for (H_Elf_Dyn *d = (H_Elf_Dyn *)dynAddr; d->d_tag != DT_NULL; d++) {
        if (d->d_tag == DT_HASH) hash = (H_Elf_Addr)d->d_un.d_ptr;
        else if (d->d_tag == DT_GNU_HASH) gnu = (H_Elf_Addr)d->d_un.d_ptr;
        else if (d->d_tag == DT_STRTAB) str = (H_Elf_Addr)d->d_un.d_ptr;
        else if (d->d_tag == DT_STRSZ) strsz = (size_t)d->d_un.d_val;
        else if (d->d_tag == DT_SYMTAB) sym = (H_Elf_Addr)d->d_un.d_ptr;
        else if (d->d_tag == DT_SYMENT) syment = (size_t)d->d_un.d_val;
    }
    size_t symCount = hash ? ((const H_Elf_Word *)hash)[1] : 0;
    if (hash) wipeRegion((void *)hash, hashTableSize(hash), PROT_READ);
    if (gnu) wipeRegion((void *)gnu, gnuHashSize(gnu, sym, syment, str), PROT_READ);
    if (sym && symCount) wipeRegion((void *)sym, symCount * syment, PROT_READ);
    else if (sym && str > sym) wipeRegion((void *)sym, (size_t)(str - sym), PROT_READ);
    if (str && strsz) wipeRegion((void *)str, strsz, PROT_READ);
    wipeRegion((void *)dynAddr, (size_t)dynPhdr->p_memsz, PROT_READ);
    return true;
}

inline bool mangleElf(uintptr_t base) {
    const H_Elf_Ehdr *eh = (const H_Elf_Ehdr *)base;
    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) { HIDEE("bad elf magic %p", (void *)base); return false; }
    const H_Elf_Phdr *phdr = (const H_Elf_Phdr *)(base + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++) {
        if (phdr[i].p_type == PT_NOTE && phdr[i].p_memsz) wipeRegion((void *)(base + phdr[i].p_vaddr), (size_t)phdr[i].p_memsz, PROT_READ);
        if (phdr[i].p_type == PT_DYNAMIC) wipeDynamic(base, &phdr[i]);
    }
    size_t phdrBytes = (size_t)eh->e_phnum * eh->e_phentsize;
    wipeRegion((void *)base, sizeof(H_Elf_Ehdr), PROT_READ | PROT_EXEC);
    wipeRegion((void *)(base + eh->e_phoff), phdrBytes, PROT_READ | PROT_EXEC);
    HIDEL("ehdr+phdr %p", (void *)base);
    LoadRange range{};
    if (loadRange(base, &range)) wipeGuardPages(&range);
    HIDEL("mangle done base=%p", (void *)base);
    return true;
}

inline bool manglePlugin(void *handle) {
    if (!handle) return false;
    void *sym = dlsym(handle, "JNI_OnLoad");
    if (!sym) { HIDEE("dlsym JNI_OnLoad: %s", dlerror()); return false; }
    Dl_info info{};
    if (!dladdr(sym, &info) || !info.dli_fbase) { HIDEE("dladdr failed"); return false; }
    return mangleElf((uintptr_t)info.dli_fbase);
}

}
