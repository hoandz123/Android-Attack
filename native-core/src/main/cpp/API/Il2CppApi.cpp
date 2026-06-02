#include "Il2CppApi.h"
#include <dlfcn.h>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include "xdl.h"
#include "IL2CPP_API.hpp"
#include "Tools/Tools.h"

void *il2cpp_handle = nullptr;
std::atomic<bool> il2cpp_loaded{false};


// Helper: Split namespace and class name from FullName ("xxx.abc" -> "xxx", "abc")
void SplitNamespaceClass(const std::string &fullName, std::string &outNamespace, std::string &outClass) {
    size_t pos = fullName.rfind('.');
    if (pos == std::string::npos) {
        outNamespace = "";
        outClass = fullName;
    } else {
        outNamespace = fullName.substr(0, pos);
        outClass = fullName.substr(pos + 1);
    }
}

// Hỗ trợ: Lấy tất cả assemblies trong domain
std::vector<Il2CppAssembly *> GetAllAssemblies() {
    std::vector<Il2CppAssembly *> result;
    Il2CppDomain *domain = il2cpp_domain_get();
    if (!domain) return result;
    il2cpp_thread_attach(domain);
    size_t size = 0;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(domain, &size);
    for (size_t i = 0; i < size; ++i) {
        result.push_back(assemblies[i]);
    }
    return result;
}


// Hỗ trợ: Tìm class trong 1 assembly
Il2CppClass *FindClassInAssembly(Il2CppAssembly *assembly, const std::string &ns, const std::string &cls) {
    if (!assembly) return nullptr;
    Il2CppImage *image = assembly->get_image();
    if (!image) return nullptr;
    return image->class_from_name(ns.c_str(), cls.c_str());
}


std::unordered_map<std::string, Il2CppClass *> GetAllIl2CppClasses() {
    std::unordered_map<std::string, Il2CppClass *> class_map;

    auto push = [&](Il2CppClass *c) {
        if (!c) return;
        class_map[c->get_full_name_all()] = c;
    };

    auto asms = GetAllAssemblies();
    for (auto *a: asms) {
        if (auto img = a->get_image()) {
            for (size_t i = 0, n = img->get_class_count(); i < n; ++i) {
                Il2CppClass *c = img->get_class(i);
                if (!c) continue;
                push(c);
                {
                    void *it = nullptr;
                    Il2CppClass *k;
                    while ((k = c->get_nested_types(&it))) push(k);
                }
                {
                    void *it = nullptr;
                    Il2CppClass *k;
                    while ((k = c->get_interfaces(&it))) push(k);
                }
                {
                    void *it = nullptr;
                    Il2CppField *f;
                    while ((f = c->get_fields(&it))) {
                        push(f->get_type()->get_class());
                    }
                }
                {
                    void *it = nullptr;
                    Il2CppMethod *m;
                    while ((m = c->get_methods(&it))) {
                        push(m->get_return_type()->get_class());
                        uint8_t pc = m->get_param_count();
                        for (uint32_t pi = 0; pi < pc; ++pi) {
                            push(m->get_param(pi)->get_class());
                        }
                    }
                }
                Class *parent = c;
                while ((parent = parent->get_parent())) {
                    push(parent);
                }
            }
        }
    }
    return class_map;
}


// Hỗ trợ: Tìm class trong tất cả assemblies
Il2CppClass *FindClass(const std::string &FullName) {
    static std::unordered_map<std::string, Il2CppClass *> class_cache;
    static std::mutex cache_mutex;
    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = class_cache.find(FullName);
        if (it != class_cache.end()) return it->second;
    }

    std::string ns, cls;
    SplitNamespaceClass(FullName, ns, cls);


    auto assemblies = GetAllAssemblies();
    for (auto *assembly: assemblies) {
        Il2CppClass *klass = FindClassInAssembly(assembly, ns, cls);
        if (klass) {
            std::lock_guard<std::mutex> lock(cache_mutex);
            class_cache[FullName] = klass;
            return klass;
        }
    }


    static std::unordered_map<std::string, Il2CppClass *> all = GetAllIl2CppClasses();
    Class *klass = all[FullName];
    if (klass) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        class_cache[FullName] = klass;
        return klass;
    }
    SDKLOGE("FindClass: Không tìm thấy class %s", FullName.c_str());
    return nullptr;
}


Il2CppImage *Il2CppGetImageByName(const char *image) {
    size_t size;

    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);

    for (int i = 0; i < size; ++i) {
        auto *img = const_cast<Il2CppImage *>(il2cpp_assembly_get_image(assemblies[i]));
        const char *img_name = il2cpp_image_get_name(img);

        if (strcmp(img_name, image) == 0) {
            return img;
        }
    }
    return nullptr;
}


Il2CppClass *Il2CppGetClassType(const char *image, const char *namespaze, const char *clazz) {
    static std::unordered_map<std::string, Il2CppClass *> cache;

    std::string s = image;
    s += namespaze;
    s += clazz;
    if (cache.count(s) > 0)
        return cache[s];

    Il2CppImage *img = Il2CppGetImageByName(image);
    if (!img) {
        return nullptr;
    }


    Il2CppClass *klass = il2cpp_class_from_name(img, namespaze, clazz);
    if (!klass) {
        SDKLOGE("Il2CppGetClassType: Không tìm thấy class %s trong image %s", clazz, image);
        return nullptr;
    }

    cache[s] = klass;
    return klass;
}

void IL2Cpp::Il2CppGetStaticFieldValue(const char *image, const char *namespaze, const char *clazz, const char *name, void *output) {
    il2cpp_thread_attach(il2cpp_domain_get());
    Il2CppClass *klass = Il2CppGetClassType(image, namespaze, clazz);
    if (!klass) {
        return;
    }
    Il2CppField *field = il2cpp_class_get_field_from_name(klass, name);
    if (!field) {
        return;
    }
    il2cpp_field_static_get_value(field, output);
}

void IL2Cpp::Il2CppGetStaticFieldValue(const char *namespaze, const char *clazz, const char *name, void *output) {
    static std::unordered_map<std::string, Il2CppClass *> cache;
    std::string key = std::string(namespaze) + "|" + clazz + "|" + name;
    auto it = cache.find(key);
    if (it != cache.end()) {
        Il2CppField *field = il2cpp_class_get_field_from_name(it->second, name);
        if (!field) {
            return;
        }
        il2cpp_field_static_get_value(field, output);
        return;
    }
    il2cpp_thread_attach(il2cpp_domain_get());
    size_t size;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
    for (int i = 0; i < size; ++i) {
        Il2CppImage *img = il2cpp_assembly_get_image(assemblies[i]);
        const char *img_name = il2cpp_image_get_name(img);
        Il2CppClass *klass = Il2CppGetClassType(img_name, namespaze, clazz);
        if (klass) {
            Il2CppField *field = il2cpp_class_get_field_from_name(klass, name);
            if (!field) {
                return;
            }
            il2cpp_field_static_get_value(field, output);
            cache[key] = klass;
        }
    }
}


void IL2Cpp::Il2CppGetStaticFieldValue(const char *clazz, const char *name, void *output) {
    static std::unordered_map<std::string, Il2CppClass *> cache;
    std::string key = std::string(clazz) + "|" + name;
    auto it = cache.find(key);
    if (it != cache.end()) {
        Il2CppField *field = il2cpp_class_get_field_from_name(it->second, name);
        if (!field) {
            return;
        }
        il2cpp_field_static_get_value(field, output);
        return;
    }
    il2cpp_thread_attach(il2cpp_domain_get());
    size_t size;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
    for (int i = 0; i < size; ++i) {
        Il2CppImage *image = il2cpp_assembly_get_image(assemblies[i]);
        auto classCount = il2cpp_image_get_class_count(image);
        for (int j = 0; j < classCount; ++j) {
            Il2CppClass *klass = il2cpp_image_get_class(image, j);
            if (!klass) {
                continue;
            }
            const char *nameClazz = il2cpp_class_get_name(klass);
            if (strcmp(nameClazz, clazz) == 0) {
                Il2CppField *field = il2cpp_class_get_field_from_name(klass, name);
                if (!field) {
                    return;
                }
                il2cpp_field_static_get_value(field, output);
                cache[key] = klass;
            }

        }
    }
}


void IL2Cpp::Il2CppSetStaticFieldValue(const char *image, const char *namespaze, const char *clazz, const char *name, void *value) {
    Il2CppClass *klass = Il2CppGetClassType(image, namespaze, clazz);
    if (!klass) {
        return;
    }
    Il2CppField *field = il2cpp_class_get_field_from_name(klass, name);
    if (!field) {
        return;
    }
    il2cpp_field_static_set_value(field, value);
}

void *IL2Cpp::Il2CppGetMethodOffset(const char *image, const char *namespaze, const char *clazz, const char *name, int argsCount, int index) {
//    SDKLOGI("image: %s, namespaze: %s, clazz: %s, name: %s, argsCount: %d, index: %d", image, namespaze, clazz, name, argsCount, index);
    static std::unordered_map<std::string, void *> cache;
    std::string key = std::string(image) + "|" + namespaze + "|" + clazz + "|" + name + "|" + std::to_string(argsCount) + "|" + std::to_string(index);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
//    uintptr_t il2cpp_base = 0;
//    if (il2cpp_domain_get_assemblies) {
//        Dl_info dlInfo;
//        if (dladdr((void *) il2cpp_domain_get_assemblies, &dlInfo)) {
//            il2cpp_base = reinterpret_cast<uint64_t>(dlInfo.dli_fbase);
//        }
//    }
    if (index > 0) {
        size_t size;
        Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
        int cahceIndex = 0;
        for (int i = 0; i < size; ++i) {
            Il2CppImage *imageObj = il2cpp_assembly_get_image(assemblies[i]);
            auto classCount = il2cpp_image_get_class_count(imageObj);
            for (int j = 0; j < classCount; ++j) {
                Il2CppClass *klass = il2cpp_image_get_class(imageObj, j);
                if (!klass) {
                    continue;
                }
                const char *nameClazz = il2cpp_class_get_name(klass);
                if (strcmp(nameClazz, clazz) == 0) {
                    void *iter = nullptr;
                    while (auto method = (Il2CppMethod *) il2cpp_class_get_methods(klass, &iter)) {
                        const char *className = il2cpp_class_get_name(klass);
                        const char *methodName = il2cpp_method_get_name(method);
                        uint32_t paramCount = il2cpp_method_get_param_count(method);
                        if (strcmp(className, clazz) == 0 && strcmp(methodName, name) == 0 && paramCount == argsCount) {
                            cahceIndex++;
//                            SDKLOGI("%s %s %d", className, methodName, il2cpp_method_get_param_count(method));
                            if (cahceIndex != index) {
                                continue;
                            }
//                            uintptr_t methodAddress = (uintptr_t) method->methodPointer - il2cpp_base;
//                            SDKLOGI("%s %s %d %lx", className, methodName, paramCount, methodAddress);
                            cache[key] = (void *) method->methodPointer;
                            return (void *) method->methodPointer;
                        }
                    }
                }
            }
        }
        SDKLOGE("Il2CppGetMethodOffset not found (index) %s::%s::%s::%s argsCount=%d index=%d", image, namespaze, clazz, name, argsCount, index);
        return nullptr;
    }
    Il2CppClass *klass = Il2CppGetClassType(image, namespaze, clazz);
    if (!klass) {
        SDKLOGE("Il2CppGetMethodOffset class not found %s::%s::%s", image, namespaze, clazz);
        return nullptr;
    }
    void **method = (void **) il2cpp_class_get_method_from_name(klass, name, argsCount);
    if (!method) {
        SDKLOGE("Il2CppGetMethodOffset method not found %s::%s::%s::%s argsCount=%d", image, namespaze, clazz, name, argsCount);
        return nullptr;
    }


    cache[key] = *method;
    return *method;
}
void *IL2Cpp::Il2CppGetMethodOffset(const char *namespaze, const char *clazz, const char *name, int argsCount) {
    SDKLOGI("Il2CppGetMethodOffset %s %s %s %d", namespaze, clazz, name, argsCount);
    static std::unordered_map<std::string, void *> cache;
    std::string key = std::string(namespaze) + "|" + clazz + "|" + name + "|" + std::to_string(argsCount);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    size_t size;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
    for (int i = 0; i < size; ++i) {
        Il2CppImage *img = il2cpp_assembly_get_image(assemblies[i]);
        const char *img_name = il2cpp_image_get_name(img);
        Il2CppClass *klass = Il2CppGetClassType(img_name, namespaze, clazz);
        if (klass) {
            void **method = (void **) il2cpp_class_get_method_from_name(klass, name, argsCount);
            if (method) {
                cache[key] = *method;
                return *method;
            }
        }
    }
    SDKLOGE("Il2CppGetMethodOffset not found %s %s %s %d", namespaze, clazz, name, argsCount);
    return nullptr;
}
void *IL2Cpp::Il2CppGetMethodOffset(const char *clazz, const char *name, int argsCount) {
    SDKLOGI("Il2CppGetMethodOffset %s %s %d", clazz, name, argsCount);
    static std::unordered_map<std::string, void *> cache;
    std::string key = std::string(clazz) + "|" + name + "|" + std::to_string(argsCount);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    size_t size;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
    for (int i = 0; i < size; ++i) {
        Il2CppImage *image = il2cpp_assembly_get_image(assemblies[i]);
        auto classCount = il2cpp_image_get_class_count(image);
        for (int j = 0; j < classCount; ++j) {
            Il2CppClass *klass = il2cpp_image_get_class(image, j);
            if (!klass) {
                continue;
            }
            const char *nameClazz = il2cpp_class_get_name(klass);
            if (strcmp(nameClazz, clazz) == 0) {
                void **method = (void **) il2cpp_class_get_method_from_name(klass, name, argsCount);
                if (method) {
                    cache[key] = *method;
                    return *method;
                }
            }

        }
    }
    SDKLOGE("GET_METHOD %s %s %d not found\n", clazz, name, argsCount);
    return nullptr;
}
size_t IL2Cpp::Il2CppGetFieldOffset(const char *image, const char *namespaze, const char *clazz, const char *name) {
    static std::unordered_map<std::string, size_t> cache;
    std::string key = std::string(image) + "|" + namespaze + "|" + clazz + "|" + name;

    auto it = cache.find(key);
    if (it != cache.end())
        return it->second;

    if (auto *klass = Il2CppGetClassType(image, namespaze, clazz)) {
        if (auto *field = il2cpp_class_get_field_from_name(klass, name)) {
            size_t result = il2cpp_field_get_offset(field);
            return cache[key] = result;
        }
    }

    SDKLOGE("IL2Cpp::Il2CppGetFieldOffset %s %s %s %s not found\n", image, namespaze, clazz, name);
    return static_cast<size_t>(-1);
}

size_t IL2Cpp::Il2CppGetFieldOffset(const char *namespaze, const char *clazz, const char *name) {
    static std::unordered_map<std::string, size_t> cache;
    std::string key = std::string(namespaze) + "|" + clazz + "|" + name;
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    size_t size;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
    for (int i = 0; i < size; ++i) {
        Il2CppImage *img = il2cpp_assembly_get_image(assemblies[i]);
        const char *img_name = il2cpp_image_get_name(img);
        Il2CppClass *klass = Il2CppGetClassType(img_name, namespaze, clazz);
        if (klass) {
            Il2CppField *field = il2cpp_class_get_field_from_name(klass, name);
            if (!field) {
                return -1;
            }
            auto result = il2cpp_field_get_offset(field);
            cache[key] = result;
            return result;
        }
    }
    SDKLOGE("Il2CppGetFieldOffset not found %s %s %s", namespaze, clazz, name);
    return -1;
}
size_t IL2Cpp::Il2CppGetFieldOffset(const char *clazz, const char *name) {
    static std::unordered_map<std::string, size_t> cache;
    std::string key = std::string(clazz) + "|" + name;
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    size_t size;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &size);
    for (int i = 0; i < size; ++i) {
        Il2CppImage *image = il2cpp_assembly_get_image(assemblies[i]);
        auto classCount = il2cpp_image_get_class_count(image);
        for (int j = 0; j < classCount; ++j) {
            Il2CppClass *klass = il2cpp_image_get_class(image, j);
            if (!klass) {
                continue;
            }
            const char *nameClazz = il2cpp_class_get_name(klass);
            if (strcmp(nameClazz, clazz) == 0) {
                Il2CppField *field = il2cpp_class_get_field_from_name(klass, name);
                if (field) {
                    auto result = il2cpp_field_get_offset(field);
                    cache[key] = result;
                    return result;
                }
            }

        }
    }
    SDKLOGE("IL2Cpp::Il2CppGetFieldOffset %s %s not found\n", clazz, name);
    return -1;
}


std::string Il2CppString::to_string() const noexcept {
    std::string out;
    const char16_t *data = get_chars();
    int32_t len = get_length();
    if (!data || len <= 0) return out;

    out.reserve(static_cast<size_t>(len) * 3 + 1);

    auto push_replacement = [&out]() {
        // U+FFFD -> 0xEF 0xBF 0xBD
        out.push_back(static_cast<char>(0xEF));
        out.push_back(static_cast<char>(0xBF));
        out.push_back(static_cast<char>(0xBD));
    };

    for (int32_t i = 0; i < len; ++i) {
        char32_t cp = static_cast<uint16_t>(data[i]);

        // high surrogate?
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (i + 1 < len) {
                char32_t lo = static_cast<uint16_t>(data[i + 1]);
                if (lo >= 0xDC00 && lo <= 0xDFFF) {
                    // valid pair
                    cp = ((cp - 0xD800) << 10) + (lo - 0xDC00) + 0x10000;
                    ++i;
                } else {
                    // invalid low surrogate
                    push_replacement();
                    continue;
                }
            } else {
                // unmatched high surrogate at end
                push_replacement();
                continue;
            }
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            // stray low surrogate
            push_replacement();
            continue;
        }

        // encode cp into UTF-8
        if (cp <= 0x7F) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else { // cp <= 0x10FFFF
            out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}


// Hỗ trợ: Lấy symbol từ libil2cpp.so
uintptr_t get_symbol(const char *name) {
    uintptr_t addr = (Tools::GetPackageName() == OBF("com.garena.game.kgvn")) ? (uintptr_t)il2cpp_api::get_il2cpp(name) : Tools::FindSymbol(OBF("libil2cpp.so"), name);
    if (!addr) {
        SDKLOGW("Không tìm thấy symbol: %s", name);
        return 0;
    }
    return addr;
}

namespace {

std::mutex g_il2cpp_code_index_mutex;
std::vector<std::pair<uintptr_t, Il2CppMethod *>> g_il2cpp_code_index;
bool g_il2cpp_code_index_built = false;

void Il2CppEnsureCodeIndexLocked() {
    if (g_il2cpp_code_index_built) {
        return;
    }
    g_il2cpp_code_index.clear();
    Il2CppDomain *domain = il2cpp_domain_get();
    if (!domain) {
        g_il2cpp_code_index_built = true;
        return;
    }
    il2cpp_thread_attach(domain);
    size_t nasm = 0;
    Il2CppAssembly **assemblies = il2cpp_domain_get_assemblies(domain, &nasm);
    for (size_t ai = 0; ai < nasm; ++ai) {
        Il2CppImage *img = il2cpp_assembly_get_image(assemblies[ai]);
        if (!img) {
            continue;
        }
        const size_t ncls = il2cpp_image_get_class_count(img);
        for (size_t ci = 0; ci < ncls; ++ci) {
            Il2CppClass *klass = il2cpp_image_get_class(img, ci);
            if (!klass) {
                continue;
            }
            void *iter = nullptr;
            Il2CppMethod *m = nullptr;
            while ((m = il2cpp_class_get_methods(klass, &iter))) {
                void *mp = m->methodPointer;
                if (!mp) {
                    continue;
                }
                g_il2cpp_code_index.emplace_back(reinterpret_cast<uintptr_t>(mp), m);
            }
        }
    }
    std::sort(g_il2cpp_code_index.begin(), g_il2cpp_code_index.end(),
              [](const std::pair<uintptr_t, Il2CppMethod *> &a,
                 const std::pair<uintptr_t, Il2CppMethod *> &b) { return a.first < b.first; });
    g_il2cpp_code_index.erase(
        std::unique(g_il2cpp_code_index.begin(), g_il2cpp_code_index.end(),
                    [](const std::pair<uintptr_t, Il2CppMethod *> &a,
                       const std::pair<uintptr_t, Il2CppMethod *> &b) { return a.first == b.first; }),
        g_il2cpp_code_index.end());
    g_il2cpp_code_index_built = true;
}

}  // namespace

Il2CppMethod *Il2CppFindMethodByCodeAddress(uintptr_t pc) {
    if (!il2cpp_loaded.load(std::memory_order_acquire)) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(g_il2cpp_code_index_mutex);
    Il2CppEnsureCodeIndexLocked();
    if (g_il2cpp_code_index.empty()) {
        return nullptr;
    }
    size_t lo = 0;
    size_t hi = g_il2cpp_code_index.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (g_il2cpp_code_index[mid].first <= pc) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo == 0) {
        return nullptr;
    }
    return g_il2cpp_code_index[lo - 1].second;
}

// Kiểm tra il2cpp đã load chưa
bool check_il2cpp_loaded() {
    SDKLOGI("Kiểm tra il2cpp đã load...");
    bool loaded = get_symbol("il2cpp_domain_get") != 0;
    if (loaded) {
        SDKLOGI("il2cpp đã load");
    }
    return loaded;
}

#undef DO_API
#define DO_API(r, n, p) r (*n) p

#include "Il2cpp_Symbol.h"

// Khởi tạo và gán địa chỉ symbol il2cpp
void Init_Il2cpp_Symbol() {
    SDKLOGI("Bắt đầu chờ il2cpp load...");
    while (!check_il2cpp_loaded()) {
        sleep(1);
    }
    sleep(3); // Đợi game ổn định

    SDKLOGI("Gán địa chỉ symbol il2cpp...");
#undef DO_API
#define DO_API(r, n, p)                     \
    {                                           \
        (n) = (r(*) p)get_symbol(#n);             \
        if(!(n)) SDKLOGI("API Not Found: %s ", #n);  \
    }

#include "Il2cpp_Symbol.h"

    SDKLOGI("Đã gán xong symbol, bắt đầu thao tác il2cpp...");
    il2cpp_loaded = true;

    while (!il2cpp_is_vm_thread(nullptr)) {
        SDKLOGD("Waiting for il2cpp to attach");
        sleep(1);
    }


}
