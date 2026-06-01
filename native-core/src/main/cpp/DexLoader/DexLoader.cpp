#include "DexLoader.hpp"

#define LOG_TAG OBF("DexLoader")
#include <Includes/Logger.h>

#include "JniReflect.hpp"
#include <JNIHelper/JNIHelper.hpp>

#include <cstdio>
#include <string>
#include <string.h>
#include <unistd.h>

namespace dex_loader {

static constexpr int kMinSdkForInMemoryDex = 26;
static constexpr int kInitRetryCount = 30;
static constexpr useconds_t kInitRetryDelayUs = 20000; // 20ms

static jobject FieldGet(JNIEnv *env, jobject field, jobject instance) {
    jclass field_cls = env->FindClass(OBF("java/lang/reflect/Field"));
    jmethodID get = env->GetMethodID(field_cls, OBF("get"), OBF("(Ljava/lang/Object;)Ljava/lang/Object;"));
    return env->CallObjectMethod(field, get, instance);
}

static void FieldSet(JNIEnv *env, jobject field, jobject instance, jobject value) {
    jclass field_cls = env->FindClass(OBF("java/lang/reflect/Field"));
    jmethodID set = env->GetMethodID(field_cls, OBF("set"), OBF("(Ljava/lang/Object;Ljava/lang/Object;)V"));
    env->CallVoidMethod(field, set, instance, value);
}

static jobject MethodInvoke(JNIEnv *env, jobject method, jobject instance, jobjectArray args) {
    jclass method_cls = env->FindClass(OBF("java/lang/reflect/Method"));
    jmethodID invoke = env->GetMethodID(method_cls, OBF("invoke"), OBF("(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;"));
    return env->CallObjectMethod(method, invoke, instance, args);
}

static jobjectArray MakeBufferArray(JNIEnv *env, const std::vector<std::vector<uint8_t>> &dex_files) {
    jclass buffer_cls = env->FindClass(OBF("java/nio/ByteBuffer"));
    const jsize n = static_cast<jsize>(dex_files.size());
    jobjectArray arr = env->NewObjectArray(n, buffer_cls, nullptr);
    for (jsize i = 0; i < n; ++i) {
        const auto &blob = dex_files[static_cast<size_t>(i)];
        if (blob.empty()) continue;
        jobject buf = env->NewDirectByteBuffer(const_cast<void *>(static_cast<const void *>(blob.data())),
                                                static_cast<jlong>(blob.size()));
        env->SetObjectArrayElement(arr, i, buf);
        env->DeleteLocalRef(buf);
    }
    return arr;
}

static jobject NewSuppressedList(JNIEnv *env) {
    jclass array_list = env->FindClass(OBF("java/util/ArrayList"));
    jmethodID ctor = env->GetMethodID(array_list, OBF("<init>"), OBF("()V"));
    return env->NewObject(array_list, ctor);
}

static int SdkInt(JNIEnv *env) {
    jclass build = env->FindClass(OBF("android/os/Build$VERSION"));
    if (!build || env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    jfieldID fid = env->GetStaticFieldID(build, OBF("SDK_INT"), OBF("I"));
    if (!fid || env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    return env->GetStaticIntField(build, fid);
}

static void LogClassName(JNIEnv *env, jobject obj, const char *prefix) {
    if (!obj) return;
    jclass cls = env->GetObjectClass(obj);
    jclass class_cls = env->FindClass(OBF("java/lang/Class"));
    jmethodID get_name = env->GetMethodID(class_cls, OBF("getName"), OBF("()Ljava/lang/String;"));
    jstring jname = (jstring)env->CallObjectMethod(cls, get_name);
    if (!jname || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    const char *name = env->GetStringUTFChars(jname, nullptr);
    LOGI(OBF("%s %s"), prefix, name ? name : OBF("?"));
    if (name) env->ReleaseStringUTFChars(jname, name);
}

static void LogClassLoaderChain(JNIEnv *env, jobject context) {
    if (!context) return;
    jclass context_cls = env->FindClass(OBF("android/content/Context"));
    jmethodID get_cl = env->GetMethodID(context_cls, OBF("getClassLoader"), OBF("()Ljava/lang/ClassLoader;"));
    jobject loader = env->CallObjectMethod(context, get_cl);
    if (!loader || env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE(OBF("ClassLoader chain: unavailable"));
        return;
    }

    jclass cl_cls = env->FindClass(OBF("java/lang/ClassLoader"));
    jmethodID get_parent = env->GetMethodID(cl_cls, OBF("getParent"), OBF("()Ljava/lang/ClassLoader;"));
    int depth = 0;
    while (loader && depth < 12) {
        char label[48];
        snprintf(label, sizeof(label), "ClassLoader[%d]", depth);
        LogClassName(env, loader, label);
        const bool has_path = jni_reflect::HasFieldObject(env, loader, OBF("pathList"));
        LOGI(OBF("  pathList=%s"), has_path ? OBF("yes") : OBF("no"));

        jobject parent = env->CallObjectMethod(loader, get_parent);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            break;
        }
        env->DeleteLocalRef(loader);
        loader = parent;
        ++depth;
    }
    if (loader) env->DeleteLocalRef(loader);
}

/** Android 10+ thường bọc PathClassLoader (DelegateLastClassLoader…) — đi parent đến loader có pathList. */
static jobject ResolveLoaderWithPathList(JNIEnv *env, jobject context) {
    jclass context_cls = env->FindClass(OBF("android/content/Context"));
    jmethodID get_cl = env->GetMethodID(context_cls, OBF("getClassLoader"), OBF("()Ljava/lang/ClassLoader;"));
    jobject loader = env->CallObjectMethod(context, get_cl);
    if (!loader || env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE(OBF("getClassLoader failed"));
        return nullptr;
    }

    jclass cl_cls = env->FindClass(OBF("java/lang/ClassLoader"));
    jmethodID get_parent = env->GetMethodID(cl_cls, OBF("getParent"), OBF("()Ljava/lang/ClassLoader;"));
    jobject candidate = env->NewLocalRef(loader);

    for (int depth = 0; depth < 12 && candidate; ++depth) {
        if (jni_reflect::HasFieldObject(env, candidate, OBF("pathList"))) {
            if (depth > 0) {
                LOGI(OBF("using parent ClassLoader at depth %d for pathList"), depth);
            }
            jobject out = env->NewLocalRef(candidate);
            env->DeleteLocalRef(candidate);
            env->DeleteLocalRef(loader);
            return out;
        }
        jobject parent = env->CallObjectMethod(candidate, get_parent);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            break;
        }
        env->DeleteLocalRef(candidate);
        candidate = parent;
    }

    if (candidate) env->DeleteLocalRef(candidate);
    env->DeleteLocalRef(loader);
    LOGE(OBF("no ClassLoader in chain exposes pathList"));
    return nullptr;
}

static jobject WaitForApplication(JNIEnv *env) {
    jclass at = env->FindClass(OBF("android/app/ActivityThread"));
    jmethodID current_app = env->GetStaticMethodID(at, OBF("currentApplication"), OBF("()Landroid/app/Application;"));
    if (!current_app) return nullptr;

    for (int i = 0; i < kInitRetryCount; ++i) {
        jobject app = env->CallStaticObjectMethod(at, current_app);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            app = nullptr;
        }
        if (app) {
            if (i > 0) LOGI(OBF("currentApplication ready after %d retries"), i);
            return app;
        }
        usleep(kInitRetryDelayUs);
    }
    return nullptr;
}

static bool VerifyEmbeddedClass(JNIEnv *env) {
    jclass bridge = jni::FindClass(env, OBF("com/android/attack/nativedex/ActivityTrackerBridge"));
    if (!bridge) {
        LOGE(OBF("verify: ActivityTrackerBridge not visible"));
        return false;
    }
    jmethodID install = env->GetStaticMethodID(bridge, OBF("install"), OBF("()Z"));
    if (!install || env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE(OBF("verify: install() not found on loaded class"));
        return false;
    }
    LOGI(OBF("verify: ActivityTrackerBridge OK"));

    jclass overlay = jni::FindClass(env, OBF("com/android/attack/nativedex/EglOverlay"));
    if (!overlay) {
        LOGE(OBF("verify: EglOverlay not visible — :native-dex:generateEmbeddedDex && rebuild :app"));
        return false;
    }
    LOGI(OBF("verify: EglOverlay OK"));

    jclass touch = jni::FindClass(env, OBF("com/android/attack/nativedex/TouchInputBridge"));
    if (!touch) {
        LOGE(OBF("verify: TouchInputBridge not visible"));
        return false;
    }
    LOGI(OBF("verify: TouchInputBridge OK"));

    jclass keyboard = jni::FindClass(env, OBF("com/android/attack/nativedex/KeyboardInputBridge"));
    if (!keyboard) {
        LOGE(OBF("verify: KeyboardInputBridge not visible"));
        return false;
    }
    LOGI(OBF("verify: KeyboardInputBridge OK"));
    return true;
}

bool LoadIntoContext(JNIEnv *env, jobject context, const std::vector<std::vector<uint8_t>> &dex_files) {
    if (!env || !context || dex_files.empty()) return false;

    const int sdk = SdkInt(env);
    if (sdk > 0 && sdk < kMinSdkForInMemoryDex) {
        LOGE(OBF("API %d < %d: makeInMemoryDexElements unavailable"), sdk, kMinSdkForInMemoryDex);
        return false;
    }

    if (sdk >= 29) {
        LogClassLoaderChain(env, context);
    }

    jobject class_loader = ResolveLoaderWithPathList(env, context);
    if (!class_loader) return false;
    LogClassName(env, class_loader, OBF("dex inject target"));

    jobject path_list_field = jni_reflect::FindFieldObject(env, class_loader, OBF("pathList"));
    if (!path_list_field) {
        env->DeleteLocalRef(class_loader);
        return false;
    }
    jobject path_list = FieldGet(env, path_list_field, class_loader);
    if (!path_list) {
        env->DeleteLocalRef(class_loader);
        return false;
    }

    jobject dex_elements_field = jni_reflect::FindFieldObject(env, path_list, OBF("dexElements"));
    if (!dex_elements_field) {
        env->DeleteLocalRef(class_loader);
        return false;
    }
    jobject dex_elements = FieldGet(env, dex_elements_field, path_list);
    if (!dex_elements) {
        env->DeleteLocalRef(class_loader);
        return false;
    }

    jobject make_dex_elements = jni_reflect::FindMethod(env, path_list, OBF("makeInMemoryDexElements"), OBF("([Ljava/nio/ByteBuffer;Ljava/util/List;)"));
    if (!make_dex_elements) {
        LOGE(OBF("makeInMemoryDexElements missing on this device (API %d)"), sdk);
        env->DeleteLocalRef(class_loader);
        return false;
    }

    jobjectArray buffers = MakeBufferArray(env, dex_files);
    jobject suppressed = NewSuppressedList(env);
    jobjectArray invoke_args = env->NewObjectArray(2, env->FindClass(OBF("java/lang/Object")), nullptr);
    env->SetObjectArrayElement(invoke_args, 0, buffers);
    env->SetObjectArrayElement(invoke_args, 1, suppressed);

    jobject add_elements = MethodInvoke(env, make_dex_elements, path_list, invoke_args);
    if (!add_elements || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        LOGE(OBF("makeInMemoryDexElements failed (API %d)"), sdk);
        env->DeleteLocalRef(class_loader);
        return false;
    }

    auto *old_arr = (jobjectArray)dex_elements;
    auto *add_arr = (jobjectArray)add_elements;
    const jsize old_len = env->GetArrayLength(old_arr);
    const jsize add_len = env->GetArrayLength(add_arr);

    if (env->ExceptionCheck()) env->ExceptionClear();

    jclass array_cls = env->FindClass(OBF("java/lang/reflect/Array"));
    jmethodID new_instance =
        env->GetStaticMethodID(array_cls, OBF("newInstance"), OBF("(Ljava/lang/Class;I)Ljava/lang/Object;"));
    jclass cls_cls = env->FindClass(OBF("java/lang/Class"));
    jmethodID get_component = env->GetMethodID(cls_cls, OBF("getComponentType"), OBF("()Ljava/lang/Class;"));
    jclass dex_array_cls = env->GetObjectClass(old_arr);
    jobject component = env->CallObjectMethod(dex_array_cls, get_component);
    jobject new_elements_obj =
        env->CallStaticObjectMethod(array_cls, new_instance, component, old_len + add_len);
    if (!new_elements_obj || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        LOGE(OBF("Array.newInstance failed"));
        env->DeleteLocalRef(class_loader);
        return false;
    }
    auto *new_elements = (jobjectArray)new_elements_obj;

    for (jsize i = 0; i < old_len; ++i) {
        jobject item = env->GetObjectArrayElement(old_arr, i);
        env->SetObjectArrayElement(new_elements, i, item);
    }
    for (jsize i = 0; i < add_len; ++i) {
        jobject item = env->GetObjectArrayElement(add_arr, i);
        env->SetObjectArrayElement(new_elements, old_len + i, item);
    }

    FieldSet(env, dex_elements_field, path_list, new_elements);
    env->DeleteLocalRef(class_loader);
    LOGI(OBF("injected %d dex (%d + %d elements) API %d"), static_cast<int>(dex_files.size()), old_len, add_len, sdk);
    return true;
}

bool LoadIntoContext(JNIEnv *env, jobject context, const uint8_t *dex, size_t size) {
    std::vector<std::vector<uint8_t>> one;
    one.emplace_back(dex, dex + size);
    return LoadIntoContext(env, context, one);
}

bool Init(JavaVM *vm, const uint8_t *dex, size_t size) {
    if (!vm || !dex || size == 0) return false;
    static bool s_inited = false;
    if (s_inited) return true;

    if (!jni::Inited() && !jni::Init(vm)) return false;
    JNIEnv *env = jni::Env();
    if (!env) return false;

    const int sdk = SdkInt(env);
    if (sdk > 0) {
        LOGI(OBF("init SDK %d (min in-memory dex API %d)"), sdk, kMinSdkForInMemoryDex);
        if (sdk < kMinSdkForInMemoryDex) return false;
    }

    jobject app = WaitForApplication(env);
    if (!app) {
        LOGE(OBF("currentApplication unavailable after %d retries"), kInitRetryCount);
        return false;
    }

    if (!LoadIntoContext(env, app, dex, size)) return false;
    if (!VerifyEmbeddedClass(env)) return false;

    s_inited = true;
    LOGI(OBF("init ok (%zu bytes)"), size);
    return true;
}

} // namespace dex_loader
