//
// Created by TEAMHMG on 02/09/2025.
//

#pragma once
#ifndef IL2CPP_IL2CPP_STRUCT_H
#define IL2CPP_IL2CPP_STRUCT_H

#include <sstream>
#include <cstddef>
#include <vector>
#include <unordered_map>
#include <dlfcn.h>
#include <fstream>
#include <ctime>
#include <string>
#include <jni.h>
#include "il2cpp-tabledefs.h"
#include "obfuscate.h"

// Forward declarations

struct Il2CppString;

Il2CppClass *FindClass(const std::string& FullName);
void Init_Il2cpp_Symbol();

#undef DO_API
#define DO_API(r, n, p) extern r (*n) p

#include "Il2cpp_Symbol.h"




template<typename U>
inline void* to_voidptr(U* p) { return (void*)p; }
template<typename U>
inline void* to_voidptr(U& v) { return (void*)std::addressof(v); }

template<typename T, typename... Args>
typename std::enable_if<std::is_void<T>::value, void>::type
call_method(Il2CppMethod* m, void* inst, Args&&... args) {
    constexpr std::size_t count = sizeof...(Args);
    void* params[sizeof...(Args)] = { to_voidptr(std::forward<Args>(args))... };
    il2cpp_runtime_invoke(m, inst, count ? params : nullptr, nullptr);
}

template<typename T, typename... Args>
typename std::enable_if<std::is_pointer<T>::value, T>::type
call_method(Il2CppMethod* m, void* inst, Args&&... args) {
    constexpr std::size_t count = sizeof...(Args);
    void* params[sizeof...(Args)] = { to_voidptr(std::forward<Args>(args))... };
    Il2CppObject* raw = il2cpp_runtime_invoke(m, inst, count ? params : nullptr, nullptr);
    return raw ? reinterpret_cast<T>(raw) : nullptr;
}

template<typename T, typename... Args>
typename std::enable_if<!std::is_void<T>::value && !std::is_pointer<T>::value, T>::type
call_method(Il2CppMethod* m, void* inst, Args&&... args) {
    constexpr std::size_t count = sizeof...(Args);
    void* params[sizeof...(Args)] = { to_voidptr(std::forward<Args>(args))... };
    Il2CppObject* raw = il2cpp_runtime_invoke(m, inst, count ? params : nullptr, nullptr);
    return raw ? *static_cast<T*>(il2cpp_object_unbox(raw)) : T{};
}

typedef enum Il2CppTypeEnum {
    IL2CPP_TYPE_END = 0x00,       /* End of List */
    IL2CPP_TYPE_VOID = 0x01,
    IL2CPP_TYPE_BOOLEAN = 0x02,
    IL2CPP_TYPE_CHAR = 0x03,
    IL2CPP_TYPE_I1 = 0x04,
    IL2CPP_TYPE_U1 = 0x05,
    IL2CPP_TYPE_I2 = 0x06,
    IL2CPP_TYPE_U2 = 0x07,
    IL2CPP_TYPE_I4 = 0x08,
    IL2CPP_TYPE_U4 = 0x09,
    IL2CPP_TYPE_I8 = 0x0a,
    IL2CPP_TYPE_U8 = 0x0b,
    IL2CPP_TYPE_R4 = 0x0c,
    IL2CPP_TYPE_R8 = 0x0d,
    IL2CPP_TYPE_STRING = 0x0e,
    IL2CPP_TYPE_PTR = 0x0f,       /* arg: <type> token */
    IL2CPP_TYPE_BYREF = 0x10,       /* arg: <type> token */
    IL2CPP_TYPE_VALUETYPE = 0x11,       /* arg: <type> token */
    IL2CPP_TYPE_CLASS = 0x12,       /* arg: <type> token */
    IL2CPP_TYPE_VAR = 0x13,       /* Generic parameter in a generic type definition, represented as number (compressed unsigned integer) number */
    IL2CPP_TYPE_ARRAY = 0x14,       /* type, rank, boundsCount, bound1, loCount, lo1 */
    IL2CPP_TYPE_GENERICINST = 0x15,     /* <type> <type-arg-count> <type-1> \x{2026} <type-n> */
    IL2CPP_TYPE_TYPEDBYREF = 0x16,
    IL2CPP_TYPE_I = 0x18,
    IL2CPP_TYPE_U = 0x19,
    IL2CPP_TYPE_FNPTR = 0x1b,        /* arg: full method signature */
    IL2CPP_TYPE_OBJECT = 0x1c,
    IL2CPP_TYPE_SZARRAY = 0x1d,       /* 0-based one-dim-array */
    IL2CPP_TYPE_MVAR = 0x1e,       /* Generic parameter in a generic method definition, represented as number (compressed unsigned integer)  */
    IL2CPP_TYPE_CMOD_REQD = 0x1f,       /* arg: typedef or typeref token */
    IL2CPP_TYPE_CMOD_OPT = 0x20,       /* optional arg: typedef or typref token */
    IL2CPP_TYPE_INTERNAL = 0x21,       /* CLR internal type */

    IL2CPP_TYPE_MODIFIER = 0x40,       /* Or with the following types */
    IL2CPP_TYPE_SENTINEL = 0x41,       /* Sentinel for varargs method signature */
    IL2CPP_TYPE_PINNED = 0x45,       /* Local var that points to pinned object */

    IL2CPP_TYPE_ENUM = 0x55        /* an enumeration */
} Il2CppTypeEnum;


typedef struct Il2CppGenericInst {
    uint32_t type_argc;
    Il2CppType **type_argv;
} Il2CppGenericInst;

typedef struct Il2CppGenericContext {
    Il2CppGenericInst *class_inst;
    Il2CppGenericInst *method_inst;
} Il2CppGenericContext;

typedef struct Il2CppGenericClass {
    int32_t typeDefinitionIndex;
    Il2CppGenericContext context;
    Il2CppClass *cached_class;
} Il2CppGenericClass;


struct MetadataField {
    uint32_t offset;
    uint32_t typeIndex;
    const char *name;
    bool isStatic;
};

enum class MetadataTypeFlags : uint32_t {
    kNone = 0,
    kValueType = 1 << 0,
    kArray = 1 << 1,
    kArrayRankMask = 0xFFFF0000
};

// Thông tin về một kiểu trong metadata
struct MetadataType {
    MetadataTypeFlags flags;
    MetadataField *fields;
    uint32_t fieldCount;
    uint32_t staticsSize;
    uint8_t *statics;
    uint32_t baseOrElementTypeIndex;
    char *name;
    const char *assemblyName;
    uint64_t typeInfoAddress;
    uint32_t size;
};

struct MetadataSnapshot {
    uint32_t typeCount;
    MetadataType *types;
};

struct ManagedMemorySection {
    uint64_t sectionStartAddress;
    uint32_t sectionSize;
    uint8_t *sectionBytes;
};


struct ManagedHeap {
    uint32_t sectionCount;
    ManagedMemorySection *sections;
};

struct Stacks {
    uint32_t stackCount;
    ManagedMemorySection *stacks;
};

struct NativeObject {
    uint32_t gcHandleIndex;
    uint32_t size;
    uint32_t instanceId;
    uint32_t classId;
    uint32_t referencedNativeObjectIndicesCount;
    uint32_t *referencedNativeObjectIndices;
};

struct GCHandles {
    uint32_t trackedObjectCount;
    uint64_t *pointersToObjects;
};

struct RuntimeInformation {
    uint32_t pointerSize;
    uint32_t objectHeaderSize;
    uint32_t arrayHeaderSize;
    uint32_t arrayBoundsOffsetInHeader;
    uint32_t arraySizeOffsetInHeader;
    uint32_t allocationGranularity;
};

// Snapshot tổng
struct ManagedMemorySnapshot {
    ManagedHeap heap;
    Stacks stacks;
    MetadataSnapshot metadata;
    GCHandles gcHandles;
    RuntimeInformation runtimeInformation;
    void *additionalUserInformation;
};


struct Il2CppMemoryCallbacks {
    void* (*malloc_func)(size_t size);
    void* (*aligned_malloc_func)(size_t size, size_t alignment);
    void (*free_func)(void *ptr);
    void (*aligned_free_func)(void *ptr);
    void* (*calloc_func)(size_t nmemb, size_t size);
    void* (*realloc_func)(void *ptr, size_t size);
    void* (*aligned_realloc_func)(void *ptr, size_t size, size_t alignment);
};


struct Il2CppObject {
    Il2CppClass *klass;
    void *monitor;

    Il2CppClass *get_class() const {
        return il2cpp_object_get_class(const_cast<Il2CppObject *>(this));
    }
    uint32_t get_size() const {
        return il2cpp_object_get_size(const_cast<Il2CppObject *>(this));
    }
    Il2CppMethod *get_virtual_method(Il2CppMethod *method) const {
        return il2cpp_object_get_virtual_method(const_cast<Il2CppObject *>(this), const_cast<Il2CppMethod *>(method));
    }
    void *unbox() const {
        return il2cpp_object_unbox(const_cast<Il2CppObject *>(this));
    }

    void runtime_object_init() {
        il2cpp_runtime_object_init(const_cast<Il2CppObject *>(this));
    }

    template<typename T>
    T get_field_value(const char *fieldName) const;

    template<typename T>
    T get_field_object(const char *fieldName) const;

    template<typename T>
    void set_field_value(const char *fieldName, T value);

    template<typename T>
    void set_field_object(const char *fieldName, T value);

    template<typename T, typename... Args>
    T invoke_method(const char *name, Args &&... args) const;

    template<typename T, typename... Args>
    T invoke_method(int index, const char *name, Args &&... args) const;

    inline std::string ObjectToJson() const;
};

using Object = Il2CppObject;


struct Il2CppReflectionType {
    Il2CppObject object;
    Il2CppType *type;
};


typedef struct Il2CppArrayType {
    Il2CppType *etype;
    uint8_t rank;
    uint8_t numsizes;
    uint8_t numlobounds;
    int *sizes;
    int *lobounds;
} Il2CppArrayType;


struct Il2CppType {
    union {
        void *dummy;
        int32_t klassIndex; /* for VALUETYPE and CLASS */
        Il2CppType *type;   /* for PTR and SZARRAY */
        Il2CppArrayType *array; /* for ARRAY */
        int32_t genericParameterIndex; /* for VAR and MVAR */
        Il2CppGenericClass *generic_class; /* for GENERICINST */
    } data;

    unsigned int attrs: 16;
    Il2CppTypeEnum type: 8;
    unsigned int num_mods: 6;
    unsigned int byref: 1;
    unsigned int pinned: 1;


    Il2CppObject *get_object() const {
        return il2cpp_type_get_object(const_cast<Il2CppType *>(this));
    }
    int get_type() const {
        return il2cpp_type_get_type(const_cast<Il2CppType *>(this));
    }
    Il2CppClass *get_class() const {
        return il2cpp_class_from_type(const_cast<Il2CppType *>(this));
    }
    Il2CppClass *get_class_or_element_class() const {
        return il2cpp_type_get_class_or_element_class(const_cast<Il2CppType *>(this));
    }
    const char *get_name() const {
        return il2cpp_type_get_name(const_cast<Il2CppType *>(this));
    }
    bool is_byref() const {
        return il2cpp_type_is_byref(const_cast<Il2CppType *>(this));
    }
    uint32_t get_attrs() const {
        return il2cpp_type_get_attrs(const_cast<Il2CppType *>(this));
    }
    bool equals(Il2CppType *otherType) const {
        return il2cpp_type_equals(const_cast<Il2CppType *>(this), const_cast<Il2CppType *>(otherType));
    }
    char *get_assembly_qualified_name() const {
        return il2cpp_type_get_assembly_qualified_name(const_cast<Il2CppType *>(this));
    }
    bool is_static() const {
        return il2cpp_type_is_static(const_cast<Il2CppType *>(this));
    }
};

using Type = Il2CppType;


struct Il2CppMethod {
    void *methodPointer;

    const char *get_name() const {
        return il2cpp_method_get_name(const_cast<Il2CppMethod *>(this));
    }

    uintptr_t get_offset() const {
        if (!methodPointer) return 0;
        Dl_info info;
        if (dladdr(methodPointer, &info) == 0) {
            return 0;
        }
        uintptr_t base = reinterpret_cast<uintptr_t>(info.dli_fbase);
        uintptr_t func = reinterpret_cast<uintptr_t>(methodPointer);
        return func - base;
    }

    inline const char *get_signature_name();

    bool is_generic() const {
        return il2cpp_method_is_generic(const_cast<Il2CppMethod *>(this));
    }
    bool is_inflated() const {
        return il2cpp_method_is_inflated(const_cast<Il2CppMethod *>(this));
    }
    bool is_instance() const {
        return il2cpp_method_is_instance(const_cast<Il2CppMethod *>(this));
    }
    uint32_t get_param_count() const {
        return il2cpp_method_get_param_count(const_cast<Il2CppMethod *>(this));
    }
    Il2CppType *get_param(uint32_t index) const {
        return il2cpp_method_get_param(const_cast<Il2CppMethod *>(this), index);
    }
    Il2CppClass *get_class() const {
        return il2cpp_method_get_class(const_cast<Il2CppMethod *>(this));
    }
    const char *get_param_name(uint32_t index) const {
        return il2cpp_method_get_param_name(const_cast<Il2CppMethod *>(this), index);
    }
    Il2CppType *get_return_type() const {
        return il2cpp_method_get_return_type(const_cast<Il2CppMethod *>(this));
    }
    uint32_t get_flags(uint32_t *iflags) const {
        return il2cpp_method_get_flags(const_cast<Il2CppMethod *>(this), iflags);
    }
    uint32_t get_token() const {
        return il2cpp_method_get_token(const_cast<Il2CppMethod *>(this));
    }

    template<typename T, typename... Args>
    T invoke(void *instance, Args &&... args) {
//        SDKLOGD("invoke method: %s", get_signature_name());
        return call_method<T>(this, instance, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T static_invoke(Args &&... args) {
//        SDKLOGD("invoke method: %s", get_signature_name());
        return call_method<T>(this, nullptr, std::forward<Args>(args)...);
    }
};

using Method = Il2CppMethod;

struct Il2CppField {
    size_t get_offset() const {
        return il2cpp_field_get_offset(const_cast<Il2CppField *>(this));
    }
    const char *get_name() const {
        return il2cpp_field_get_name(const_cast<Il2CppField *>(this));
    }
    Il2CppType *get_type() const {
        return il2cpp_field_get_type(const_cast<Il2CppField *>(this));
    }
    void static_get_value(void *out) const {
        il2cpp_field_static_get_value(const_cast<Il2CppField *>(this), out);
    }
    void static_set_value(void *value) const {
        il2cpp_field_static_set_value(const_cast<Il2CppField *>(this), value);
    }
    int get_flags() const {
        return il2cpp_field_get_flags(const_cast<Il2CppField *>(this));
    }
};

using Field = Il2CppField;

struct Il2CppDomain {
    Il2CppAssembly *open_assembly(const char *name) const {
        return il2cpp_domain_assembly_open(const_cast<Il2CppDomain *>(this), name);
    }
    Il2CppAssembly **get_assemblies(size_t *size) const {
        return il2cpp_domain_get_assemblies(const_cast<Il2CppDomain *>(this), size);
    }
    Il2CppThread *attach_thread() const {
        return il2cpp_thread_attach(const_cast<Il2CppDomain *>(this));
    }

    // Dump toàn bộ runtime IL2CPP (header + tất cả class của mọi assembly) ra file.
    // filepath rỗng → dùng <persistentDataPath>/packageName_versionName_[ABI].cs;
    //                  fallback /sdcard/Download/... nếu lấy không được.
    // Trả về path đã ghi; empty string nếu fail.
    inline static std::string dump_domain(const std::string &filepath = "");
};

using Domain = Il2CppDomain;

struct Il2CppAssembly {
    Il2CppImage *get_image() const {
        return il2cpp_assembly_get_image(const_cast<Il2CppAssembly *>(this));
    }
};

using Assembly = Il2CppAssembly;

struct Il2CppThread {
    bool is_vm_thread() const {
        return il2cpp_is_vm_thread(const_cast<Il2CppThread *>(this));
    }
};

using Thread = Il2CppThread;

struct Il2CppImage {
    const char *get_name() const {
        return il2cpp_image_get_name(const_cast<Il2CppImage *>(this));
    }
    size_t get_class_count() const {
        return il2cpp_image_get_class_count(const_cast<Il2CppImage *>(this));
    }
    Il2CppClass *get_class(size_t index) const {
        return il2cpp_image_get_class(const_cast<Il2CppImage *>(this), index);
    }
    Il2CppAssembly *get_assembly() const {
        return il2cpp_image_get_assembly(const_cast<Il2CppImage *>(this));
    }
    Il2CppClass *class_from_name(const char *namespaze, const char *name) const {
        return il2cpp_class_from_name(const_cast<Il2CppImage *>(this), namespaze, name);
    }

    // Dump tất cả class trong image theo định dạng Il2CppDumper.
    // Định nghĩa out-of-line vì cần Il2CppClass::dump_class() đã complete.
    inline std::string dump_image() const;
};

using Image = Il2CppImage;

struct Il2CppClass {
    std::string get_name() const {
        return il2cpp_class_get_name(const_cast<Il2CppClass *>(this));
    }
    std::string get_namespace() const {
        return il2cpp_class_get_namespace(const_cast<Il2CppClass *>(this));
    }
    std::string get_full_name() const {
        std::string ns = get_namespace();
        std::string name = get_name();
        std::stringstream ss;
        ss << ns << (ns.length() ? "." : "") << name;
        return ss.str().c_str();
    }
    std::string get_full_name_all() {
        std::string fullName = get_full_name();
        Type *type = get_type();
        if (type->get_type() == IL2CPP_TYPE_GENERICINST) {
            const Il2CppGenericClass *genericClass = type->data.generic_class;
            if (!genericClass) return "";
            size_t pos = fullName.find('`');
            if (pos != std::string::npos) fullName = fullName.substr(0, pos);
            const Il2CppGenericInst *genericInst = genericClass->context.class_inst;
            if (genericInst && genericInst->type_argc > 0) {
                fullName += "<";
                for (uint32_t gi = 0; gi < genericInst->type_argc; ++gi) {
                    if (gi > 0) fullName += ",";
                    Type *param_type = genericInst->type_argv[gi];
                    if (!param_type) continue;
                    Il2CppClass *param_klass = param_type->get_class();
                    if (!param_klass) continue;
                    if (param_type->get_type() == IL2CPP_TYPE_GENERICINST) {
                        fullName += param_klass->get_full_name_all();
                        continue;
                    }
                    fullName += param_klass->get_full_name();
                }
                fullName += ">";
            }
        }
        return fullName;
    }
    std::string get_field_name(int index) {
        void *iter = nullptr;
        Il2CppField *field = nullptr;
        int i = 0;
        while ((field = get_fields(&iter)) != nullptr) {
            if (i == index) {
                return field->get_name();
            }
            ++i;
        }
        return "";
    }
    template<typename T>
    std::string get_enum_name(T value) {
        if (!is_enum()) return "";
        Il2CppClass* underlyingType = get_element_class();
        if (!underlyingType) return "";
        Type* underlyingIl2CppType = underlyingType->get_type();
        if (!underlyingIl2CppType) return "";
        auto baseType = underlyingIl2CppType->get_type();
        void *iter = nullptr;
        Il2CppField *field = nullptr;
        while ((field = get_fields(&iter)) != nullptr) {
            if (strcmp(field->get_name(), "value__") == 0) continue;
            uint64_t enum_value = 0;
            field->static_get_value(&enum_value);
            switch (baseType) {
                case IL2CPP_TYPE_I1:  if ((int8_t)enum_value  == (int8_t)value)  return field->get_name(); break;
                case IL2CPP_TYPE_U1:  if ((uint8_t)enum_value == (uint8_t)value) return field->get_name(); break;
                case IL2CPP_TYPE_I2:  if ((int16_t)enum_value == (int16_t)value) return field->get_name(); break;
                case IL2CPP_TYPE_U2:  if ((uint16_t)enum_value== (uint16_t)value)return field->get_name(); break;
                case IL2CPP_TYPE_I4:  if ((int32_t)enum_value == (int32_t)value) return field->get_name(); break;
                case IL2CPP_TYPE_U4:  if ((uint32_t)enum_value== (uint32_t)value)return field->get_name(); break;
                case IL2CPP_TYPE_I8:  if ((int64_t)enum_value == (int64_t)value) return field->get_name(); break;
                case IL2CPP_TYPE_U8:  if ((uint64_t)enum_value== (uint64_t)value)return field->get_name(); break;
                default: break;
            }
        }
        return "";
    }
    template<typename T>
    T get_enum_value(const char *name) {
        if (!is_enum()) return T{};
        void *iter = nullptr;
        Il2CppField *field = nullptr;
        while ((field = get_fields(&iter)) != nullptr) {
            if (strcmp(field->get_name(), name) == 0) {
                T value{};
                field->static_get_value(&value);
                return value;
            }
        }
        return T{};
    }


    template<typename T>
    T get_static_field_value(const char *fieldName) {
        Il2CppField *field = get_field_from_name(fieldName);
        if (!field) {
            return T{};
        }
        T value{};
        field->static_get_value(&value);
        return value;
    }

    template<typename T>
    void set_static_field_value(const char *fieldName, T value) {
        Il2CppField *field = get_field_from_name(fieldName);
        if (!field) {
            return;
        }
        field->static_set_value(&value);
    }


    Il2CppObject* value_box(void* data) {
        return il2cpp_value_box(const_cast<Il2CppClass *>(this), data);
    }

    Il2CppImage *get_image() const {
        return il2cpp_class_get_image(const_cast<Il2CppClass *>(this));
    }
    Il2CppClass *get_parent() const {
        return il2cpp_class_get_parent(const_cast<Il2CppClass *>(this));
    }
    bool is_valuetype() const {
        return il2cpp_class_is_valuetype(const_cast<Il2CppClass *>(this));
    }
    bool is_enum() const {
        return il2cpp_class_is_enum(const_cast<Il2CppClass *>(this));
    }
    bool is_generic() const {
        return il2cpp_class_is_generic(const_cast<Il2CppClass *>(this));
    }
    bool is_inflated() const {
        return il2cpp_class_is_inflated(const_cast<Il2CppClass *>(this));
    }
    bool is_assignable_from(Il2CppClass *other) const {
        return il2cpp_class_is_assignable_from(const_cast<Il2CppClass *>(this), const_cast<Il2CppClass *>(other));
    }
    uint32_t get_flags() const {
        return il2cpp_class_get_flags(const_cast<Il2CppClass *>(this));
    }
    bool is_abstract() const {
        return il2cpp_class_is_abstract(const_cast<Il2CppClass *>(this));
    }
    bool is_interface() const {
        return il2cpp_class_is_interface(const_cast<Il2CppClass *>(this));
    }
    int get_rank() const {
        return il2cpp_class_get_rank(const_cast<Il2CppClass *>(this));
    }
    bool has_attribute(Il2CppClass *attr_class) const {
        return il2cpp_class_has_attribute(const_cast<Il2CppClass *>(this), const_cast<Il2CppClass *>(attr_class));
    }
    bool has_references() const {
        return il2cpp_class_has_references(const_cast<Il2CppClass *>(this));
    }
    uint32_t get_type_token() const {
        return il2cpp_class_get_type_token(const_cast<Il2CppClass *>(this));
    }
    Il2CppType *get_type() const {
        return il2cpp_class_get_type(const_cast<Il2CppClass *>(this));
    }
    Il2CppClass *get_declaring_type() const {
        return il2cpp_class_get_declaring_type(const_cast<Il2CppClass *>(this));
    }
    int32_t instance_size() const {
        return il2cpp_class_instance_size(const_cast<Il2CppClass *>(this));
    }
    size_t num_fields() const {
        return il2cpp_class_num_fields(const_cast<Il2CppClass *>(this));
    }
    int32_t value_size(uint32_t *align) const {
        return il2cpp_class_value_size(const_cast<Il2CppClass *>(this), align);
    }
    Il2CppClass *get_element_class() const {
        return il2cpp_class_get_element_class(const_cast<Il2CppClass *>(this));
    }
    Il2CppField *get_field_from_name(const char *name) const {
        return il2cpp_class_get_field_from_name(const_cast<Il2CppClass *>(this), name);
    }
    Il2CppField *get_fields(void **iter = nullptr) const {
        return il2cpp_class_get_fields(const_cast<Il2CppClass *>(this), iter);
    }

    // Dump tất cả field theo định dạng Il2CppDumper:
    //   public static <Type> <Name>; // 0xOFFSET
    //   private <Type> <Name>;       // 0xOFFSET
    //   public const <Type> <Name>;  // (literal, no offset)
    std::string dump_fields() const {
        std::ostringstream ss;
        void *iter = nullptr;
        Il2CppField *field = nullptr;
        while ((field = il2cpp_class_get_fields(const_cast<Il2CppClass *>(this), &iter)) != nullptr) {
            const int flags = field->get_flags();
            const int access = flags & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;

            const char *modifier = "private";
            switch (access) {
                case FIELD_ATTRIBUTE_PRIVATE:        modifier = "private"; break;
                case FIELD_ATTRIBUTE_FAM_AND_ASSEM:  modifier = "private protected"; break;
                case FIELD_ATTRIBUTE_ASSEMBLY:       modifier = "internal"; break;
                case FIELD_ATTRIBUTE_FAMILY:         modifier = "protected"; break;
                case FIELD_ATTRIBUTE_FAM_OR_ASSEM:   modifier = "protected internal"; break;
                case FIELD_ATTRIBUTE_PUBLIC:         modifier = "public"; break;
                default:                              modifier = "private"; break;
            }

            const bool is_static  = (flags & FIELD_ATTRIBUTE_STATIC) != 0;
            const bool is_literal = (flags & FIELD_ATTRIBUTE_LITERAL) != 0;

            const char *type_name_raw = nullptr;
            Il2CppType *type = field->get_type();
            if (type) type_name_raw = il2cpp_type_get_name(type);
            std::string type_name = type_name_raw ? type_name_raw : "?";

            const char *name = field->get_name();
            if (!name) name = "?";

            ss << "\t" << modifier;
            if (is_literal)     ss << " const";
            else if (is_static) ss << " static";
            ss << " " << type_name << " " << name << ";";
            if (!is_literal) ss << " // 0x" << std::hex << field->get_offset() << std::dec;
            ss << "\n";
        }
        return ss.str();
    }
    Il2CppClass *get_nested_types(void **iter = nullptr) const {
        return il2cpp_class_get_nested_types(const_cast<Il2CppClass *>(this), iter);
    }
    Il2CppClass *get_interfaces(void **iter = nullptr) const {
        return il2cpp_class_get_interfaces(const_cast<Il2CppClass *>(this), iter);
    }
    Il2CppObject *new_object() {
        Il2CppObject* obj = il2cpp_object_new(const_cast<Il2CppClass *>(this));
        if (obj) {
            obj->runtime_object_init();
        }
        return obj;
    }
    Il2CppMethod *get_methods(void **iter = nullptr) const {
        return il2cpp_class_get_methods(const_cast<Il2CppClass *>(this), iter);
    }
    Il2CppMethod *get_method(const char *name, int param_count = 0) const {
        return il2cpp_class_get_method_from_name(const_cast<Il2CppClass *>(this), name, param_count);
    }

    // Dump tất cả method theo định dạng Il2CppDumper:
    //   private System.Void Awake(); // 0xRVA
    //   public System.Boolean IsEnterSafeArea(UnityEngine.Transform transform); // 0xRVA
    //   public static System.Void Foo(System.Int32 x); // 0xRVA
    std::string dump_methods() const {
        std::ostringstream ss;
        void *iter = nullptr;
        Il2CppMethod *method = nullptr;
        while ((method = il2cpp_class_get_methods(const_cast<Il2CppClass *>(this), &iter)) != nullptr) {
            uint32_t iflags = 0;
            const uint32_t flags = method->get_flags(&iflags);
            const uint32_t access = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;

            const char *modifier = "private";
            switch (access) {
                case METHOD_ATTRIBUTE_PRIVATE:        modifier = "private"; break;
                case METHOD_ATTRIBUTE_FAM_AND_ASSEM:  modifier = "private protected"; break;
                case METHOD_ATTRIBUTE_ASSEM:          modifier = "internal"; break;
                case METHOD_ATTRIBUTE_FAMILY:         modifier = "protected"; break;
                case METHOD_ATTRIBUTE_FAM_OR_ASSEM:   modifier = "protected internal"; break;
                case METHOD_ATTRIBUTE_PUBLIC:         modifier = "public"; break;
                default:                              modifier = "private"; break;
            }

            const bool is_static = (flags & METHOD_ATTRIBUTE_STATIC) != 0;

            const char *ret_name_raw = nullptr;
            Il2CppType *ret = method->get_return_type();
            if (ret) ret_name_raw = il2cpp_type_get_name(ret);
            std::string ret_name = ret_name_raw ? ret_name_raw : "?";

            const char *name = method->get_name();
            if (!name) name = "?";

            ss << "\t" << modifier;
            if (is_static) ss << " static";
            ss << " " << ret_name << " " << name << "(";

            const uint32_t pcount = method->get_param_count();
            for (uint32_t i = 0; i < pcount; ++i) {
                if (i > 0) ss << ", ";
                const char *ptype_raw = nullptr;
                Il2CppType *ptype = method->get_param(i);
                if (ptype) ptype_raw = il2cpp_type_get_name(ptype);
                const char *pname = method->get_param_name(i);
                ss << (ptype_raw ? ptype_raw : "?") << " " << (pname ? pname : "?");
            }
            ss << ");";
            ss << " // 0x" << std::hex << method->get_offset() << std::dec;
            ss << "\n";
        }
        return ss.str();
    }

    // Dump toàn bộ khai báo class theo định dạng Il2CppDumper:
    //   // <AssemblyName>
    //   public class Foo : Bar, IBaz
    //   {
    //       // Fields
    //       ...
    //
    //       // Methods
    //       ...
    //   }
    std::string dump_class() const {
        std::ostringstream ss;

        const char *img_name = nullptr;
        if (Il2CppImage *img = get_image()) img_name = il2cpp_image_get_name(img);
        ss << "// " << (img_name ? img_name : "?") << "\n";

        const uint32_t flags = static_cast<uint32_t>(get_flags());
        const uint32_t vis = flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
        const char *modifier = "internal";
        switch (vis) {
            case TYPE_ATTRIBUTE_PUBLIC:
            case TYPE_ATTRIBUTE_NESTED_PUBLIC:        modifier = "public"; break;
            case TYPE_ATTRIBUTE_NESTED_PRIVATE:       modifier = "private"; break;
            case TYPE_ATTRIBUTE_NESTED_FAMILY:        modifier = "protected"; break;
            case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM: modifier = "private protected"; break;
            case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:  modifier = "protected internal"; break;
            case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
            case TYPE_ATTRIBUTE_NOT_PUBLIC:
            default:                                  modifier = "internal"; break;
        }

        const bool is_iface  = is_interface();
        const bool is_enum_  = is_enum();
        const bool is_val    = is_valuetype();
        const bool plain_cls = !is_iface && !is_enum_ && !is_val;
        const bool is_abs    = (flags & TYPE_ATTRIBUTE_ABSTRACT) != 0;
        const bool is_seal   = (flags & TYPE_ATTRIBUTE_SEALED)   != 0;

        const char *kind = "class";
        if (is_iface)      kind = "interface";
        else if (is_enum_) kind = "enum";
        else if (is_val)   kind = "struct";

        ss << modifier;
        if (plain_cls) {
            if (is_abs && is_seal) ss << " static";
            else if (is_abs)       ss << " abstract";
            else if (is_seal)      ss << " sealed";
        }
        ss << " " << kind << " " << get_full_name();

        std::vector<std::string> bases;
        if (plain_cls) {
            if (Il2CppClass *p = get_parent()) {
                std::string pname = p->get_full_name();
                if (!pname.empty() && pname != "System.Object") bases.push_back(pname);
            }
        }
        {
            void *iter = nullptr;
            Il2CppClass *iface = nullptr;
            while ((iface = il2cpp_class_get_interfaces(const_cast<Il2CppClass *>(this), &iter)) != nullptr) {
                std::string in = iface->get_full_name();
                if (!in.empty()) bases.push_back(in);
            }
        }
        if (!bases.empty()) {
            ss << " : ";
            for (size_t i = 0; i < bases.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << bases[i];
            }
        }
        ss << "\n{\n";

        const std::string fields_block  = dump_fields();
        const std::string methods_block = dump_methods();

        if (!fields_block.empty()) {
            ss << "\t// Fields\n" << fields_block;
        }
        if (!fields_block.empty() && !methods_block.empty()) {
            ss << "\n";
        }
        if (!methods_block.empty()) {
            ss << "\t// Methods\n" << methods_block;
        }
        ss << "}\n";
        return ss.str();
    }

    Il2CppMethod *find_method(const char *name, int param_count = 0, int index = 0) const {
//        SDKLOGI("Finding method %s with %d params in class %s (index %d)", name, param_count, get_full_name(), index);

        static std::unordered_map<std::string, Il2CppMethod*> cache;

        auto makeKey = [&](const char* clsName) -> std::string {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s:%s:%d:%d", clsName, name, param_count, index);
            return std::string(buf);
        };

        std::string key = makeKey(get_full_name().c_str());
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;

        auto scan_class_methods = [&](Il2CppClass *cls) -> Il2CppMethod * {
            void *iter = nullptr;
            Il2CppMethod *mt = nullptr;
            int found = 0;
            while ((mt = cls->get_methods(&iter)) != nullptr) {
                const char *mname = mt->get_name();
                int mparam = mt->get_param_count();
                if (mname && strcmp(mname, name) == 0 && mparam == param_count) {
                    if (found == index) {
                        return mt;
                    }
                    ++found;
                }
            }
            return nullptr;
        };


        Il2CppMethod *method = index == 0 ? get_method(name, param_count) : scan_class_methods(const_cast<Il2CppClass *>(this));
        if (!method) {
            Il2CppClass *cur = get_parent();
            while (cur) {
                method = index == 0 ? cur->get_method(name, param_count) : scan_class_methods(const_cast<Il2CppClass *>(cur));
                if (method) break;
                cur = cur->get_parent();
            }
        }
        if (!method) {
            Il2CppClass *walker = const_cast<Il2CppClass *>(this);
            while (walker) {
                void *iter = nullptr;
                Il2CppClass *iface = nullptr;
                while ((iface = walker->get_interfaces(&iter)) != nullptr) {
                    method = index == 0 ? iface->get_method(name, param_count) : scan_class_methods(const_cast<Il2CppClass *>(iface));
                    if (method) break;
                    void *iter2 = nullptr;
                    Il2CppClass *iface2 = nullptr;
                    while ((iface2 = iface->get_interfaces(&iter2)) != nullptr) {
                        method = index == 0 ? iface2->get_method(name, param_count) : scan_class_methods(const_cast<Il2CppClass *>(iface2));
                        if (method) break;
                    }
                }
                walker = walker->get_parent();
            }
        }
        if (method) {
            cache[key] = method;  // save to cache
            return method;
        }
        SDKLOGE("Method %s with %d params not found in class %s", name, param_count, get_full_name().c_str());
        return nullptr;
    }

    std::vector<Il2CppObject *> FindStaticObject() {
        std::vector<Il2CppObject *> res;

        auto reg = [](Il2CppObject **arr, int size, void *userdata) {
            if (!arr || size <= 0 || !userdata) return;
            auto v = reinterpret_cast<std::vector<Il2CppObject *> *>(userdata);
            v->insert(v->end(), arr, arr + size);
        };
        auto r_alloc = [](void *ptr, size_t size, void *userdata) -> void * {
            (void) userdata;
            if (ptr != nullptr && size == 0) {
                il2cpp_free(ptr);
                return nullptr;
            }
            return il2cpp_alloc(size);
        };

        void *state = il2cpp_unity_liveness_allocate_struct(
                this, 0,
                (il2cpp_register_object_callback) reg,
                &res,
                (il2cpp_liveness_reallocate_callback) r_alloc
        );

        if (!state) return res;
        il2cpp_unity_liveness_calculation_from_statics(state);
        il2cpp_unity_liveness_finalize(state);
        il2cpp_unity_liveness_free_struct(state);
        return res;
    }


    std::vector<Il2CppObject *> FindMemoryObjects() {
        std::vector<Il2CppObject *> out;
        ManagedMemorySnapshot *snapshot = il2cpp_capture_memory_snapshot();
        if (!snapshot) return out;
        auto& handles = snapshot->gcHandles;
        if (handles.trackedObjectCount == 0 || !handles.pointersToObjects)
            return out;

        for (uint32_t i = 0; i < handles.trackedObjectCount; ++i) {
            auto objectAddr = handles.pointersToObjects[i];
            if (!objectAddr) continue;

            auto* obj = reinterpret_cast<Il2CppObject*>(objectAddr);
            if (obj->klass == this) {
                out.push_back(obj);
            }
        }

        il2cpp_free_captured_memory_snapshot(snapshot);
        return out;
    }

};

using Class = Il2CppClass;


inline std::string Il2CppImage::dump_image() const {
    std::ostringstream ss;
    const char *img_name = get_name();
    ss << "// Image: " << (img_name ? img_name : "?") << "\n\n";
    const size_t n = get_class_count();
    for (size_t i = 0; i < n; ++i) {
        Il2CppClass *cls = get_class(i);
        if (!cls) continue;
        ss << cls->dump_class() << "\n";
    }
    return ss.str();
}


template<typename T>
struct Il2CppArray : public Il2CppObject {
    void *bounds;
    int max_length;
    T m_Items[1];
    int getLength() const {
        return max_length;
    }

    T *getPointer() const {
        return (T *) m_Items;
    }

    T &operator[](int i) {
        return m_Items[i];
    }

    T &operator[](int i) const {
        return m_Items[i];
    }

    std::size_t size() const noexcept { return static_cast<std::size_t>(getLength()); }
    bool empty() const noexcept { return size() == 0; }

    T *begin() noexcept { return getPointer(); }
    T *end() noexcept { return getPointer() + size(); }

    const T *begin() const noexcept { return getPointer(); }
    const T *end() const noexcept { return getPointer() + size(); }

    const T *cbegin() const noexcept { return getPointer(); }
    const T *cend() const noexcept { return getPointer() + size(); }

    T *data() noexcept { return getPointer(); }
    const T *data() const noexcept { return getPointer(); }
};

template<typename T>
using Array = Il2CppArray<T>;


struct Il2CppString : public Il2CppObject {
    static constexpr int32_t MAX_REASONABLE_LENGTH = 1000000; // tune as needed

    int32_t get_length() const {
        int32_t len = il2cpp_string_length(const_cast<Il2CppString *>(this));
        if (len < 0) return 0;
        if (len > MAX_REASONABLE_LENGTH) return 0;
        return len;
    }
    const char16_t *get_chars() const {
        return reinterpret_cast<const char16_t *>(il2cpp_string_chars(const_cast<Il2CppString *>(this)));
    }
    std::string to_string() const noexcept;

    static Il2CppString *Create(const char *str) {
        return il2cpp_string_new(str);
    }
};

using String = Il2CppString;

namespace il2cpp_dump_detail {

inline const char *abi_name() {
#if defined(__aarch64__)
    return "arm64-v8a";
#elif defined(__arm__)
    return "armeabi-v7a";
#elif defined(__x86_64__)
    return "x86_64";
#elif defined(__i386__)
    return "x86";
#else
    return "unknown";
#endif
}

inline std::string now_datetime() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}



inline std::string invoke_app_string_getter(const char *getter_name) {
    Il2CppClass *cls = FindClass("UnityEngine.Application");
    if (!cls) {
        SDKLOGW("[dump_domain] UnityEngine.Application class not found (looking for %s)", getter_name);
        return "";
    }
    Il2CppMethod *m = il2cpp_class_get_method_from_name(cls, getter_name, 0);
    if (!m) {
        SDKLOGW("[dump_domain] method %s not found on UnityEngine.Application", getter_name);
        return "";
    }
    void *exc = nullptr;
    Il2CppObject *ret = il2cpp_runtime_invoke(m, nullptr, nullptr, &exc);
    if (exc) {
        SDKLOGW("[dump_domain] %s threw exception", getter_name);
        return "";
    }
    if (!ret) {
        SDKLOGW("[dump_domain] %s returned null", getter_name);
        return "";
    }
    std::string result = reinterpret_cast<Il2CppString *>(ret)->to_string();
    if (result.empty()) {
        SDKLOGW("[dump_domain] %s returned empty string", getter_name);
        return "";
    }
    SDKLOGI("[dump_domain] %s => '%s'", getter_name, result.c_str());
    return result;
}



// Bridge tới JavaVM bằng cách invoke UnityEngine.AndroidJNI.GetJavaVM() (return IntPtr).
inline JavaVM *fetch_java_vm(Il2CppDomain *domain) {
    Il2CppClass *cls = FindClass("UnityEngine.AndroidJNI");
    if (!cls) {
        SDKLOGW("[dump_domain] UnityEngine.AndroidJNI class not found");
        return nullptr;
    }
    Il2CppMethod *m = cls->get_method("GetJavaVM");
    if (!m) {
        SDKLOGW("[dump_domain] AndroidJNI.GetJavaVM not found");
        return nullptr;
    }
    void *exc = nullptr;
    Il2CppObject *ret = il2cpp_runtime_invoke(m, nullptr, nullptr, &exc);
    if (exc || !ret) {
        SDKLOGW("[dump_domain] GetJavaVM invoke failed");
        return nullptr;
    }
    void *unboxed = il2cpp_object_unbox(ret);
    if (!unboxed) return nullptr;
    JavaVM *vm = *reinterpret_cast<JavaVM **>(unboxed);
    SDKLOGI("[dump_domain] JavaVM = %p", vm);
    return vm;
}

inline JavaVM *cached_java_vm(Il2CppDomain *domain) {
    static JavaVM *vm = nullptr;
    if (!vm) vm = fetch_java_vm(domain);
    return vm;
}

// Lấy app versionName qua JNI: ActivityThread.currentApplication() → PackageManager.getPackageInfo.
inline std::string get_app_version_via_jni(Il2CppDomain *domain) {
    JavaVM *vm = cached_java_vm(domain);
    if (!vm) return "";

    JNIEnv *env = nullptr;
    bool attached = false;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) {
            SDKLOGW("[dump_domain] JNI AttachCurrentThread failed");
            return "";
        }
        attached = true;
    }

    std::string result;
    auto cleanup = [&]() {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
    };

    jclass cActivityThread = env->FindClass("android/app/ActivityThread");
    if (!cActivityThread) { cleanup(); return ""; }
    jmethodID mCurrentApp = env->GetStaticMethodID(cActivityThread,
        "currentApplication", "()Landroid/app/Application;");
    if (!mCurrentApp) { cleanup(); return ""; }
    jobject app = env->CallStaticObjectMethod(cActivityThread, mCurrentApp);
    if (!app) { cleanup(); return ""; }

    jclass cContext = env->FindClass("android/content/Context");
    if (!cContext) { cleanup(); return ""; }
    jmethodID mGetPM = env->GetMethodID(cContext, "getPackageManager",
        "()Landroid/content/pm/PackageManager;");
    jmethodID mGetPkgName = env->GetMethodID(cContext, "getPackageName",
        "()Ljava/lang/String;");
    if (!mGetPM || !mGetPkgName) { cleanup(); return ""; }

    jobject pm = env->CallObjectMethod(app, mGetPM);
    jstring jPkgName = reinterpret_cast<jstring>(env->CallObjectMethod(app, mGetPkgName));
    if (!pm || !jPkgName) { cleanup(); return ""; }

    jclass cPM = env->FindClass("android/content/pm/PackageManager");
    if (!cPM) { cleanup(); return ""; }
    jmethodID mGetPI = env->GetMethodID(cPM, "getPackageInfo",
        "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    if (!mGetPI) { cleanup(); return ""; }

    jobject pi = env->CallObjectMethod(pm, mGetPI, jPkgName, 0);
    if (env->ExceptionCheck()) { env->ExceptionClear(); cleanup(); return ""; }
    if (!pi) { cleanup(); return ""; }

    jclass cPI = env->FindClass("android/content/pm/PackageInfo");
    if (!cPI) { cleanup(); return ""; }
    jfieldID fVersionName = env->GetFieldID(cPI, "versionName", "Ljava/lang/String;");
    if (!fVersionName) { cleanup(); return ""; }
    jstring jVer = reinterpret_cast<jstring>(env->GetObjectField(pi, fVersionName));
    if (jVer) {
        const char *cstr = env->GetStringUTFChars(jVer, nullptr);
        if (cstr) {
            result.assign(cstr);
            env->ReleaseStringUTFChars(jVer, cstr);
        }
    }

    cleanup();
    SDKLOGI("[dump_domain] app versionName => '%s'", result.c_str());
    return result;
}

inline std::string get_package_name_via_jni(Il2CppDomain *domain) {
    JavaVM *vm = cached_java_vm(domain);
    if (!vm) return "";

    JNIEnv *env = nullptr;
    bool attached = false;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) return "";
        attached = true;
    }

    std::string result;
    auto cleanup = [&]() {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attached) vm->DetachCurrentThread();
    };

    jclass cActivityThread = env->FindClass("android/app/ActivityThread");
    if (!cActivityThread) { cleanup(); return ""; }
    jmethodID mCurrentApp = env->GetStaticMethodID(cActivityThread,
        "currentApplication", "()Landroid/app/Application;");
    if (!mCurrentApp) { cleanup(); return ""; }
    jobject app = env->CallStaticObjectMethod(cActivityThread, mCurrentApp);
    if (!app) { cleanup(); return ""; }

    jclass cContext = env->FindClass("android/content/Context");
    if (!cContext) { cleanup(); return ""; }
    jmethodID mGetPkgName = env->GetMethodID(cContext, "getPackageName", "()Ljava/lang/String;");
    if (!mGetPkgName) { cleanup(); return ""; }

    jstring jPkg = reinterpret_cast<jstring>(env->CallObjectMethod(app, mGetPkgName));
    if (jPkg) {
        const char *cstr = env->GetStringUTFChars(jPkg, nullptr);
        if (cstr) { result.assign(cstr); env->ReleaseStringUTFChars(jPkg, cstr); }
    }

    cleanup();
    return result;
}

}  // namespace il2cpp_dump_detail

inline std::string Il2CppDomain::dump_domain(const std::string &filepath) {
    Il2CppDomain *self = il2cpp_domain_get();
    if (!self) return "";

    std::string pkg = il2cpp_dump_detail::get_package_name_via_jni(self);
    if (pkg.empty()) pkg = "unknown";
    std::string app_ver = il2cpp_dump_detail::get_app_version_via_jni(self);
    if (app_ver.empty()) app_ver = "unknown";

    std::string path = filepath;
    if (path.empty()) {
        std::string pd = il2cpp_dump_detail::invoke_app_string_getter("get_persistentDataPath");
        if (pd.empty()) pd = "/sdcard/Download";
        // packageName_versionName_[ABI].cs
        path = pd + "/" + pkg + "_" + app_ver + "_[" + il2cpp_dump_detail::abi_name() + "].cs";
    }

    size_t n = 0;
    Il2CppAssembly **arr = self->get_assemblies(&n);
    if (!arr || n == 0) return "";

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out) return "";

    std::string unity_ver = il2cpp_dump_detail::invoke_app_string_getter("get_unityVersion");
    if (unity_ver.empty()) unity_ver = "<unknown>";
    std::string product = il2cpp_dump_detail::invoke_app_string_getter("get_productName");
    if (product.empty()) product = "<unknown>";

    out << "// ============================================================\n";
    out << "//  IL2CPP Runtime Dump\n";
    out << "//  Copyright (c) Hoan\n";
    out << "// ------------------------------------------------------------\n";
    out << "//  Generated  : " << il2cpp_dump_detail::now_datetime() << "\n";
    out << "//  ABI        : " << il2cpp_dump_detail::abi_name() << "\n";
    out << "//  Output     : " << path << "\n";
    out << "//  Unity      : " << unity_ver << "\n";
    out << "//  Product    : " << product << "\n";
    out << "//  Version    : " << app_ver << "\n";
    out << "//  Assemblies : " << n << "\n";
    for (size_t i = 0; i < n; ++i) {
        if (!arr[i]) continue;
        Il2CppImage *img = arr[i]->get_image();
        const char *name = img ? img->get_name() : nullptr;
        out << "//    " << (i + 1) << ". " << (name ? name : "<?>") << "\n";
    }
    out << "// ============================================================\n\n";

    for (size_t i = 0; i < n; ++i) {
        if (!arr[i]) continue;
        Il2CppImage *img = arr[i]->get_image();
        if (!img) continue;
        out << img->dump_image() << "\n";
    }

    out.flush();
    return path;
}


template<typename T>
struct Il2CppList : public Il2CppObject {
    int get_Capacity() const {
        return this->invoke_method<int>("get_Capacity");
    }
    void set_Capacity(int value) {
        this->invoke_method<void>("set_Capacity", value);
    }
    int get_Count() const {
        return this->invoke_method<int>(OBF("get_Count"));
    }
    T get_item(int index) const {
        return this->invoke_method<T>(OBF("get_Item"), index);
    }
    void set_item(int index, T value) {
        this->invoke_method<void>(OBF("set_Item"), index, value);
    }
    void Add(T item) {
        this->invoke_method<void>("Add", item);
    }
    void Clear() {
        this->invoke_method<void>("Clear");
    }
    bool Contains(T item) const {
        return this->invoke_method<bool>("Contains", item);
    }
    bool Remove(T item) {
        return this->invoke_method<bool>("Remove", item);
    }
    void RemoveAt(int index) {
        this->invoke_method<void>("RemoveAt", index);
    }
    int IndexOf(T item) const {
        return this->invoke_method<int>("IndexOf", item);
    }
    void Insert(int index, T *item) {
        this->invoke_method<void>("Insert", index, item);
    }
    void Reverse() {
        this->invoke_method<void>("Reverse");
    }
    void Sort() {
        this->invoke_method<void>("Sort");
    }
    Array<T> *ToArray() const {
        return this->invoke_method<Array<T> *>("ToArray");
    }

    T *begin() const {
        return get_Count() == 0 ? nullptr : ToArray()->data();
    }
    T *end() const {
        auto arr = ToArray();
        return arr ? (arr->data() + arr->size()) : nullptr;
    }

    bool isEmpty() const {
        return get_Count() == 0;
    }
};

template<typename T>
using List = Il2CppList<T>;


template<typename K, typename V>
struct Entry {
    int hashCode;
    int next;
    K key;
    V value;
};

template<typename K, typename V>
struct Il2CppDictionary : public Il2CppObject {
    Array<int> *buckets;            // _buckets
    Array<Entry<K, V>> *entries;    // _entries

    int get_Count() const {
        return this->invoke_method<int>(OBF("get_Count"));
    }
    V get_Item(K key) const {
        return this->invoke_method<V>(OBF("get_Item"), key);
    }
    void set_Item(K key, V value) {
        this->invoke_method<void>(OBF("set_Item"), key, value);
    }
    bool ContainsKey(K key) const {
        return this->invoke_method<bool>("ContainsKey", key);
    }
    bool TryGetValue(K key, V *value) const {
        return this->invoke_method<bool>("TryGetValue", key, value);
    }
    bool ContainsValue(V value) const {
        return this->invoke_method<bool>("ContainsValue", value);
    }
    void Add(K key, V value) {
        this->invoke_method<void>("Add", key, value);
    }
    void Remove(K key) {
        this->invoke_method<void>("Remove", key);
    }
    void Clear() {
        this->invoke_method<void>("Clear");
    }

    K *CollectKeys() {
        int count = get_Count();
        if (count <= 0) return nullptr;
        K *outKeys = new K[count];
        if (outKeys == nullptr) return nullptr;

        int idx = 0;
        int steps = 0;
        const int maxSteps = count * 8 + 64;
        for (int i = 0; i < buckets->getLength() && idx < count && steps < maxSteps; ++i) {
            int entryIdx = buckets->getPointer()[i];
            while (entryIdx >= 0 && idx < count && steps < maxSteps) {
                steps++;
                if (entries->getPointer()[entryIdx].hashCode >= 0) {
                    outKeys[idx++] = entries->getPointer()[entryIdx].key;
                }
                entryIdx = entries->getPointer()[entryIdx].next;
            }
        }
        return outKeys;
    }

    V *CollectValues() {
        int count = get_Count();
        if (count <= 0) return nullptr;
        V *outValues = new V[count];
        if (outValues == nullptr) return nullptr;

        int idx = 0;
        int steps = 0;
        const int maxSteps = count * 8 + 64;
        for (int i = 0; i < buckets->getLength() && idx < count && steps < maxSteps; ++i) {
            int entryIdx = buckets->getPointer()[i];
            while (entryIdx >= 0 && idx < count && steps < maxSteps) {
                steps++;
                if (entries->getPointer()[entryIdx].hashCode >= 0) {
                    outValues[idx++] = entries->getPointer()[entryIdx].value;
                }
                entryIdx = entries->getPointer()[entryIdx].next;
            }
        }
        return outValues;
    }

};

template<typename K, typename V>
using Dictionary = Il2CppDictionary<K, V>;


const char *Il2CppMethod::get_signature_name() {
    uint32_t flags = 0;
    get_flags(&flags);
    std::string s;
    switch (flags & 0x0007) {
        case 0x0:
            s += "private";
            break;
        case 0x1:
            s += "private";
            break;
        case 0x2:
            s += "famandassem";
            break;
        case 0x3:
            s += "internal";
            break;
        case 0x4:
            s += "protected";
            break;
        case 0x5:
            s += "protected internal";
            break;
        case 0x6:
            s += "public";
            break;
        default:
            s += "public";
            break;
    }
    if (!is_instance()) s += " static";
    if (flags & 0x0040) s += " virtual";

    Il2CppType *rt = get_return_type();
    s += " ";
    s += rt ? (rt->get_class() ? rt->get_class()->get_full_name_all() : "void") : "void";
    s += " ";
    s += get_name() ? get_name() : "<?>";
    s += "(";
    uint32_t pc = get_param_count();
    for (uint32_t i = 0; i < pc; ++i) {
        if (i) s += ", ";
        Il2CppType *pt = get_param(i);
        const char *pn = get_param_name(i);
        s += pt ? (pt->get_class() ? pt->get_class()->get_full_name_all() : "<\?\?>") : "<\?\?>";
        if (pn && pn[0]) {
            s += " ";
            s += pn;
        }
    }
    s += ");";
    {
        std::ostringstream oss;
        oss << " // 0x" << std::hex << get_offset() << std::dec;
        s += oss.str();
    }
    return strdup(s.c_str());
}

template<typename T>
T Il2CppObject::get_field_value(const char *fieldName) const {
    Il2CppClass *kl = get_class();
    if (!kl) {
        SDKLOGE("get_field_value: !kl : %s", fieldName);
        return T();
    }
    Il2CppField *field = il2cpp_class_get_field_from_name(kl, fieldName);
    if (!field) {
        SDKLOGE("get_field_value: !field : %s - %s", kl->get_name().c_str(), fieldName);
        return T();
    }
    T value;
    il2cpp_field_get_value(const_cast<Il2CppObject *>(this), field, &value);
    return value;
}
template<typename T>
T Il2CppObject::get_field_object(const char *fieldName) const {
    Il2CppClass *kl = get_class();
    if (!kl) {
        SDKLOGE("get_field_object: !kl : %s", fieldName);
        return T();
    }
    Il2CppField *field = il2cpp_class_get_field_from_name(kl, fieldName);
    if (!field) {
//        SDKLOGE("get_field_object: !field : %s - %s", kl->get_name().c_str(), fieldName);
        return T();
    }
    return (T) il2cpp_field_get_value_object(field, const_cast<Il2CppObject *>(this));
}
template<typename T>
void Il2CppObject::set_field_value(const char *fieldName, T value) {
    Il2CppClass *kl = get_class();
    if (!kl) {
        SDKLOGE("set_field_value: !kl : %s", fieldName);
        return;
    }
    Il2CppField *field = il2cpp_class_get_field_from_name(kl, fieldName);
    if (!field) {
        SDKLOGE("set_field_value: !field : %s - %s", kl->get_name().c_str(), fieldName);
        return;
    }
    il2cpp_field_set_value(const_cast<Il2CppObject *>(this), field, &value);
}
template<typename T>
void Il2CppObject::set_field_object(const char *fieldName, T value) {
    Il2CppClass *kl = get_class();
    if (!kl) {
        SDKLOGE("set_field_object: !kl : %s", fieldName);
        return;
    }
    Il2CppField *field = il2cpp_class_get_field_from_name(kl, fieldName);
    if (!field) {
        SDKLOGE("set_field_object: !field : %s - %s", kl->get_name().c_str(), fieldName);
        return;
    }
    il2cpp_field_set_value_object(const_cast<Il2CppObject *>(this), field, (Il2CppObject *) value);
}


template<typename T, typename... Args>
T Il2CppObject::invoke_method(const char *name, Args &&... args) const {
    Class *klas = get_class();
    Il2CppMethod *method = klas->find_method(name, sizeof...(Args));
    if (!method) {
        SDKLOGE("Method %s with params not found in class %s", name, klas->get_name().c_str());
        return T();
    }
//    SDKLOGI("Invoking method: %s -> %s with %zu params", klas->get_name(), method->get_name(), sizeof...(args));
    return call_method<T>(method, (void *) this, std::forward<Args>(args)...);
}


template<typename T, typename... Args>
T Il2CppObject::invoke_method(int index, const char *name, Args &&... args) const {
    Il2CppMethod *method = get_class()->find_method(name, sizeof...(Args), index);
    if (!method) {
        SDKLOGE("Method %s with params not found in class %s", name, get_class()->get_name());
        return T();
    }
    std::string fullArgsName = "";
    for (size_t i = 0; i < method->get_param_count(); i++) {
        fullArgsName += std::string(method->get_param_name(i));
    }
//    SDKLOGI("Invoking method: %s with %zu params (%s)", method->get_name(), method->get_param_count(), fullArgsName.c_str());
    return call_method<T>(method, (void *) this, std::forward<Args>(args)...);
}


std::string Il2CppObject::ObjectToJson() const {
    Il2CppClass* cls = this->get_class();
    if (!cls) return "{}";
    std::ostringstream oss;
    oss << "{";

    void* iter = nullptr;
    Il2CppField* field = nullptr;
    bool first = true;

    while ((field = il2cpp_class_get_fields(cls, &iter))) {
        const char* name = il2cpp_field_get_name(field);
        size_t offset = il2cpp_field_get_offset(field);
//        SDKLOGI("Field: %s", name);
        if (!name || offset < 1) continue;


        if (!first) oss << ",\n";
        first = false;

        oss << "\"" << name << "\":";

        Il2CppType* type = il2cpp_field_get_type(field);
        if (!type) {
            oss << "null";
            continue;
        }
        Il2CppClass* fieldClass = type->get_class();

        switch (il2cpp_type_get_type(type)) {
            case IL2CPP_TYPE_BOOLEAN: {
                bool val = false;
                il2cpp_field_get_value((Il2CppObject*)this, field, &val);
                oss << (val ? "true" : "false");
                break;
            }
            case IL2CPP_TYPE_I1:  // sbyte
            case IL2CPP_TYPE_U1:  // byte
            case IL2CPP_TYPE_I2:  // short
            case IL2CPP_TYPE_U2:  // ushort
            case IL2CPP_TYPE_I4:  // int
            case IL2CPP_TYPE_U4:  // uint
            case IL2CPP_TYPE_I8:  // long
            case IL2CPP_TYPE_U8: { // ulong
                int64_t val = 0;
                il2cpp_field_get_value((Il2CppObject*)this, field, &val);
                oss << val;
                break;
            }
            case IL2CPP_TYPE_R4: { // float
                float val = 0;
                il2cpp_field_get_value((Il2CppObject*)this, field, &val);
                oss << val;
                break;
            }
            case IL2CPP_TYPE_R8: { // double
                double val = 0;
                il2cpp_field_get_value((Il2CppObject*)this, field, &val);
                oss << val;
                break;
            }
            case IL2CPP_TYPE_CHAR: {
                uint16_t val = 0;
                il2cpp_field_get_value((Il2CppObject*)this, field, &val);
                oss << "\"" << (char)val << "\"";
                break;
            }
            case IL2CPP_TYPE_STRING: {
                Il2CppString* str = nullptr;
                il2cpp_field_get_value((Il2CppObject*)this, field, &str);
                oss << "\"" << (str ? str->to_string() : "") << "\"";
                break;
            }
            case IL2CPP_TYPE_VALUETYPE: {
                if (fieldClass->is_enum()) {
                    int64_t val = 0;
                    il2cpp_field_get_value((Il2CppObject*)this, field, &val);
                    oss << fieldClass->get_enum_name(val); // hoặc lấy tên enum bằng il2cpp_enum_to_name nếu có
                } else {
                    Il2CppObject* refObj = nullptr;
                    il2cpp_field_get_value((Il2CppObject*)this, field, &refObj);
                    if (refObj) {
                        oss << "\"<" << il2cpp_class_get_name(refObj->klass) << ">\"";
                    } else {
                        oss << "null";
                    }
                }
                break;
            }
            default:
                Il2CppObject* refObj = nullptr;
                il2cpp_field_get_value((Il2CppObject*)this, field, &refObj);
                if (refObj) {
                    oss << "\"<" << il2cpp_class_get_name(refObj->klass) << ">\"";
                } else {
                    oss << "null";
                }
                break;
        }

    }

    oss << "}";
    return oss.str();
}


#endif //IL2CPP_IL2CPP_STRUCT_H
