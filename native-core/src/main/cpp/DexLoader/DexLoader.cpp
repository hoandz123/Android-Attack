#include "DexLoader.hpp"

#include "JniReflect.hpp"
#include <JNIHelper/JNIHelper.hpp>

#include <android/log.h>
#include <cstdio>
#include <string>
#include <string.h>
#include <unistd.h>

#define TAG "DexLoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace dex_loader {

static constexpr int kMinSdkForInMemoryDex = 26;
static constexpr int kInitRetryCount = 30;
static constexpr useconds_t kInitRetryDelayUs = 20000; // 20ms

static jobject FieldGet(JNIEnv *env, jobject field, jobject instance) {
    jclass field_cls = env->FindClass("java/lang/reflect/Field");
    jmethodID get = env->GetMethodID(field_cls, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
    return env->CallObjectMethod(field, get, instance);
}

static void FieldSet(JNIEnv *env, jobject field, jobject instance, jobject value) {
    jclass field_cls = env->FindClass("java/lang/reflect/Field");
    jmethodID set = env->GetMethodID(field_cls, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    env->CallVoidMethod(field, set, instance, value);
}

static jobject MethodInvoke(JNIEnv *env, jobject method, jobject instance, jobjectArray args) {
    jclass method_cls = env->FindClass("java/lang/reflect/Method");
    jmethodID invoke = env->GetMethodID(method_cls, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    return env->CallObjectMethod(method, invoke, instance, args);
}

static jobjectArray MakeBufferArray(JNIEnv *env, const std::vector<std::vector<uint8_t>> &dex_files) {
    jclass buffer_cls = env->FindClass("java/nio/ByteBuffer");
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
    jclass array_list = env->FindClass("java/util/ArrayList");
    jmethodID ctor = env->GetMethodID(array_list, "<init>", "()V");
    return env->NewObject(array_list, ctor);
}

static int SdkInt(JNIEnv *env) {
    jclass build = env->FindClass("android/os/Build$VERSION");
    if (!build || env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    jfieldID fid = env->GetStaticFieldID(build, "SDK_INT", "I");
    if (!fid || env->ExceptionCheck()) {
        env->ExceptionClear();
        return 0;
    }
    return env->GetStaticIntField(build, fid);
}

static void LogClassName(JNIEnv *env, jobject obj, const char *prefix) {
    if (!obj) return;
    jclass cls = env->GetObjectClass(obj);
    jclass class_cls = env->FindClass("java/lang/Class");
    jmethodID get_name = env->GetMethodID(class_cls, "getName", "()Ljava/lang/String;");
    jstring jname = (jstring)env->CallObjectMethod(cls, get_name);
    if (!jname || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    const char *name = env->GetStringUTFChars(jname, nullptr);
    LOGI("%s %s", prefix, name ? name : "?");
    if (name) env->ReleaseStringUTFChars(jname, name);
}

static void LogClassLoaderChain(JNIEnv *env, jobject context) {
    if (!context) return;
    jclass context_cls = env->FindClass("android/content/Context");
    jmethodID get_cl = env->GetMethodID(context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject loader = env->CallObjectMethod(context, get_cl);
    if (!loader || env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE("ClassLoader chain: unavailable");
        return;
    }

    jclass cl_cls = env->FindClass("java/lang/ClassLoader");
    jmethodID get_parent = env->GetMethodID(cl_cls, "getParent", "()Ljava/lang/ClassLoader;");
    int depth = 0;
    while (loader && depth < 12) {
        char label[48];
        snprintf(label, sizeof(label), "ClassLoader[%d]", depth);
        LogClassName(env, loader, label);
        const bool has_path = jni_reflect::HasFieldObject(env, loader, "pathList");
        LOGI("  pathList=%s", has_path ? "yes" : "no");

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
    jclass context_cls = env->FindClass("android/content/Context");
    jmethodID get_cl = env->GetMethodID(context_cls, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject loader = env->CallObjectMethod(context, get_cl);
    if (!loader || env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE("getClassLoader failed");
        return nullptr;
    }

    jclass cl_cls = env->FindClass("java/lang/ClassLoader");
    jmethodID get_parent = env->GetMethodID(cl_cls, "getParent", "()Ljava/lang/ClassLoader;");
    jobject candidate = env->NewLocalRef(loader);

    for (int depth = 0; depth < 12 && candidate; ++depth) {
        if (jni_reflect::HasFieldObject(env, candidate, "pathList")) {
            if (depth > 0) {
                LOGI("using parent ClassLoader at depth %d for pathList", depth);
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
    LOGE("no ClassLoader in chain exposes pathList");
    return nullptr;
}

static jobject WaitForApplication(JNIEnv *env) {
    jclass at = env->FindClass("android/app/ActivityThread");
    jmethodID current_app = env->GetStaticMethodID(at, "currentApplication", "()Landroid/app/Application;");
    if (!current_app) return nullptr;

    for (int i = 0; i < kInitRetryCount; ++i) {
        jobject app = env->CallStaticObjectMethod(at, current_app);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            app = nullptr;
        }
        if (app) {
            if (i > 0) LOGI("currentApplication ready after %d retries", i);
            return app;
        }
        usleep(kInitRetryDelayUs);
    }
    return nullptr;
}

static bool VerifyEmbeddedClass(JNIEnv *env) {
    jclass bridge = jni::FindClass(env, "com/android/attack/nativedex/ActivityTrackerBridge");
    if (!bridge) {
        LOGE("verify: ActivityTrackerBridge not visible");
        return false;
    }
    jmethodID install = env->GetStaticMethodID(bridge, "install", "()Z");
    if (!install || env->ExceptionCheck()) {
        env->ExceptionClear();
        LOGE("verify: install() not found on loaded class");
        return false;
    }
    LOGI("verify: ActivityTrackerBridge OK");

    jclass overlay = jni::FindClass(env, "com/android/attack/nativedex/EglOverlay");
    if (!overlay) {
        LOGE("verify: EglOverlay not visible — :native-dex:generateEmbeddedDex && rebuild :app");
        return false;
    }
    LOGI("verify: EglOverlay OK");

    jclass touch = jni::FindClass(env, "com/android/attack/nativedex/TouchInputBridge");
    if (!touch) {
        LOGE("verify: TouchInputBridge not visible");
        return false;
    }
    LOGI("verify: TouchInputBridge OK");

    jclass keyboard = jni::FindClass(env, "com/android/attack/nativedex/KeyboardInputBridge");
    if (!keyboard) {
        LOGE("verify: KeyboardInputBridge not visible");
        return false;
    }
    LOGI("verify: KeyboardInputBridge OK");
    return true;
}

bool LoadIntoContext(JNIEnv *env, jobject context, const std::vector<std::vector<uint8_t>> &dex_files) {
    if (!env || !context || dex_files.empty()) return false;

    const int sdk = SdkInt(env);
    if (sdk > 0 && sdk < kMinSdkForInMemoryDex) {
        LOGE("API %d < %d: makeInMemoryDexElements unavailable", sdk, kMinSdkForInMemoryDex);
        return false;
    }

    if (sdk >= 29) {
        LogClassLoaderChain(env, context);
    }

    jobject class_loader = ResolveLoaderWithPathList(env, context);
    if (!class_loader) return false;
    LogClassName(env, class_loader, "dex inject target");

    jobject path_list_field = jni_reflect::FindFieldObject(env, class_loader, "pathList");
    if (!path_list_field) {
        env->DeleteLocalRef(class_loader);
        return false;
    }
    jobject path_list = FieldGet(env, path_list_field, class_loader);
    if (!path_list) {
        env->DeleteLocalRef(class_loader);
        return false;
    }

    jobject dex_elements_field = jni_reflect::FindFieldObject(env, path_list, "dexElements");
    if (!dex_elements_field) {
        env->DeleteLocalRef(class_loader);
        return false;
    }
    jobject dex_elements = FieldGet(env, dex_elements_field, path_list);
    if (!dex_elements) {
        env->DeleteLocalRef(class_loader);
        return false;
    }

    jobject make_dex_elements = jni_reflect::FindMethod(env, path_list, "makeInMemoryDexElements",
                                                         "([Ljava/nio/ByteBuffer;Ljava/util/List;)");
    if (!make_dex_elements) {
        LOGE("makeInMemoryDexElements missing on this device (API %d)", sdk);
        env->DeleteLocalRef(class_loader);
        return false;
    }

    jobjectArray buffers = MakeBufferArray(env, dex_files);
    jobject suppressed = NewSuppressedList(env);
    jobjectArray invoke_args = env->NewObjectArray(2, env->FindClass("java/lang/Object"), nullptr);
    env->SetObjectArrayElement(invoke_args, 0, buffers);
    env->SetObjectArrayElement(invoke_args, 1, suppressed);

    jobject add_elements = MethodInvoke(env, make_dex_elements, path_list, invoke_args);
    if (!add_elements || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        LOGE("makeInMemoryDexElements failed (API %d)", sdk);
        env->DeleteLocalRef(class_loader);
        return false;
    }

    auto *old_arr = (jobjectArray)dex_elements;
    auto *add_arr = (jobjectArray)add_elements;
    const jsize old_len = env->GetArrayLength(old_arr);
    const jsize add_len = env->GetArrayLength(add_arr);

    if (env->ExceptionCheck()) env->ExceptionClear();

    jclass array_cls = env->FindClass("java/lang/reflect/Array");
    jmethodID new_instance =
        env->GetStaticMethodID(array_cls, "newInstance", "(Ljava/lang/Class;I)Ljava/lang/Object;");
    jclass cls_cls = env->FindClass("java/lang/Class");
    jmethodID get_component = env->GetMethodID(cls_cls, "getComponentType", "()Ljava/lang/Class;");
    jclass dex_array_cls = env->GetObjectClass(old_arr);
    jobject component = env->CallObjectMethod(dex_array_cls, get_component);
    jobject new_elements_obj =
        env->CallStaticObjectMethod(array_cls, new_instance, component, old_len + add_len);
    if (!new_elements_obj || env->ExceptionCheck()) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        LOGE("Array.newInstance failed");
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
    LOGI("injected %d dex (%d + %d elements) API %d", static_cast<int>(dex_files.size()), old_len, add_len, sdk);
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
        LOGI("init SDK %d (min in-memory dex API %d)", sdk, kMinSdkForInMemoryDex);
        if (sdk < kMinSdkForInMemoryDex) return false;
    }

    jobject app = WaitForApplication(env);
    if (!app) {
        LOGE("currentApplication unavailable after %d retries", kInitRetryCount);
        return false;
    }

    if (!LoadIntoContext(env, app, dex, size)) return false;
    if (!VerifyEmbeddedClass(env)) return false;

    s_inited = true;
    LOGI("init ok (%zu bytes)", size);
    return true;
}

} // namespace dex_loader
