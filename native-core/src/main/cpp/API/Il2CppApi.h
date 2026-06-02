#pragma once
#include "string"
#include <cstdint>
#include "Logger.h"
#include "Vector2.h"
#include "Vector3.h"
#include "obfuscate.h"

struct Il2CppClass;
struct Il2CppObject;
struct Il2CppType;
struct Il2CppMethod;
struct Il2CppField;
struct Il2CppImage;
struct Il2CppAssembly;
struct Il2CppThread;
struct Il2CppDomain;
struct Il2CppString;
struct ManagedMemorySnapshot;
struct Il2CppMemoryCallbacks;
struct Il2CppReflectionType;

template<typename T>
struct Il2CppArray;

template<typename T>
struct Il2CppList;


typedef void (*il2cpp_register_object_callback)(Il2CppObject **arr, int size, void *userdata);
typedef void *(*il2cpp_liveness_reallocate_callback)(void *ptr, size_t size, void *userdata);


#include "Il2cpp_Struct.h"


extern void* il2cpp_handle;
extern std::atomic<bool> il2cpp_loaded;

// PC nằm trong libil2cpp: tra method đã đăng ký (xấp xỉ theo khoảng [entry, entry_next)).
Il2CppMethod *Il2CppFindMethodByCodeAddress(uintptr_t pc);

#define EXPAND(x) x

// Field Offset
#define GET_FIELD4(image, namespaze, clazz, name) \
    IL2Cpp::Il2CppGetFieldOffset(OBF(image), OBF(namespaze), OBF(clazz), OBF(name))
#define GET_FIELD3(namespaze, clazz, name) \
    IL2Cpp::Il2CppGetFieldOffset(OBF(namespaze), OBF(clazz), OBF(name))
#define GET_FIELD2(clazz, name) \
    IL2Cpp::Il2CppGetFieldOffset(OBF(clazz), OBF(name))
#define GET_FIELD_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define GET_FIELD(...) EXPAND(GET_FIELD_MACRO(__VA_ARGS__, GET_FIELD4, GET_FIELD3, GET_FIELD2)(__VA_ARGS__))

#define GET_STATIC_FIELD4(image, namespaze, clazz, name, output) \
    IL2Cpp::Il2CppGetStaticFieldValue(OBF(image), OBF(namespaze), OBF(clazz), OBF(name), output)
#define GET_STATIC_FIELD3(namespaze, clazz, name, output) \
    IL2Cpp::Il2CppGetStaticFieldValue(OBF(namespaze), OBF(clazz), OBF(name), output)
#define GET_STATIC_FIELD2(clazz, name, output) \
    IL2Cpp::Il2CppGetStaticFieldValue(OBF(clazz), OBF(name), output)
#define GET_STATIC_FIELD_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME
#define GET_STATIC_FIELD(...) EXPAND(GET_STATIC_FIELD_MACRO(__VA_ARGS__, GET_STATIC_FIELD4, GET_STATIC_FIELD3, GET_STATIC_FIELD2)(__VA_ARGS__))


// Method Offset (bổ sung lại cho bạn nếu muốn đồng bộ)
#define GET_METHOD6(image, namespaze, clazz, name, argsCount, index) \
    IL2Cpp::Il2CppGetMethodOffset(OBF(image), OBF(namespaze), OBF(clazz), OBF(name), argsCount, index)
#define GET_METHOD5(image, namespaze, clazz, name, argsCount) \
    IL2Cpp::Il2CppGetMethodOffset(OBF(image), OBF(namespaze), OBF(clazz), OBF(name), argsCount)
#define GET_METHOD4(namespaze, clazz, name, argsCount) \
    IL2Cpp::Il2CppGetMethodOffset(OBF(namespaze), OBF(clazz), OBF(name), argsCount)
#define GET_METHOD3(clazz, name, argsCount) \
    IL2Cpp::Il2CppGetMethodOffset(OBF(clazz), OBF(name), argsCount)
#define GET_METHOD_MACRO(_1,_2,_3,_4,_5,_6,NAME,...) NAME
#define GET_METHOD(...) EXPAND(GET_METHOD_MACRO(__VA_ARGS__, GET_METHOD6, GET_METHOD5, GET_METHOD4, GET_METHOD3)(__VA_ARGS__))


namespace IL2Cpp {
    void Il2CppGetStaticFieldValue(const char *image, const char *namespaze, const char *clazz, const char *name, void *output);
    void Il2CppGetStaticFieldValue(const char *namespaze, const char *clazz, const char *name, void *output);
    void Il2CppGetStaticFieldValue(const char *clazz, const char *name, void *output);

    void Il2CppSetStaticFieldValue(const char *image, const char *namespaze, const char *clazz, const char *name, void *value);

    void *Il2CppGetMethodOffset(const char *image, const char *namespaze, const char *clazz, const char *name, int argsCount, int index = 0);
    void *Il2CppGetMethodOffset(const char *namespaze, const char *clazz, const char *name, int argsCount);
    void *Il2CppGetMethodOffset(const char *clazz, const char *name, int argsCount);


    size_t Il2CppGetFieldOffset(const char *image, const char *namespaze, const char *clazz, const char *name);
    size_t Il2CppGetFieldOffset(const char *namespaze, const char *clazz, const char *name);
    size_t Il2CppGetFieldOffset(const char *clazz, const char *name);
}

