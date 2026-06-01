#include "JNIHelper/JNIHelper.hpp"

#include <string>

namespace jni {

namespace {

JavaVM *g_vm = nullptr;

} // namespace

bool init(JavaVM *vm) {
    if (!vm) return false;
    g_vm = vm;
    return true;
}

bool inited() { return g_vm != nullptr; }

JavaVM *vm() { return g_vm; }

JNIEnv *env() {
    if (!g_vm) return nullptr;
    JNIEnv *e = nullptr;
    if (g_vm->GetEnv(reinterpret_cast<void **>(&e), kVersion) != JNI_OK) return nullptr;
    return e;
}

ScopedEnv::ScopedEnv() {
    if (!g_vm) return;
    if (g_vm->GetEnv(reinterpret_cast<void **>(&env_), kVersion) == JNI_OK) return;
    if (g_vm->AttachCurrentThread(&env_, nullptr) == JNI_OK) attached_ = true;
}

ScopedEnv::~ScopedEnv() {
    if (attached_ && g_vm) g_vm->DetachCurrentThread();
}

void clear_exception(JNIEnv *e) {
    if (e && e->ExceptionCheck()) e->ExceptionClear();
}

jclass find_class(JNIEnv *e, const char *slash_name) {
    if (!e || !slash_name) return nullptr;

    jclass cls = e->FindClass(slash_name);
    if (cls && !e->ExceptionCheck()) return cls;
    clear_exception(e);

    jclass at = e->FindClass("android/app/ActivityThread");
    if (!at || e->ExceptionCheck()) {
        clear_exception(e);
        return nullptr;
    }
    jmethodID current_app =
        e->GetStaticMethodID(at, "currentApplication", "()Landroid/app/Application;");
    jobject app = e->CallStaticObjectMethod(at, current_app);
    if (!app || e->ExceptionCheck()) {
        clear_exception(e);
        return nullptr;
    }
    jclass ctx = e->GetObjectClass(app);
    jmethodID get_cl = e->GetMethodID(ctx, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject loader = e->CallObjectMethod(app, get_cl);
    if (!loader || e->ExceptionCheck()) {
        clear_exception(e);
        return nullptr;
    }
    jclass cl_cls = e->FindClass("java/lang/ClassLoader");
    jmethodID load =
        e->GetMethodID(cl_cls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    std::string dotted = slash_name;
    for (char &c : dotted) {
        if (c == '/') c = '.';
    }
    jstring jname = e->NewStringUTF(dotted.c_str());
    jobject loaded = e->CallObjectMethod(loader, load, jname);
    if (!loaded || e->ExceptionCheck()) {
        clear_exception(e);
        return nullptr;
    }
    return reinterpret_cast<jclass>(loaded);
}

bool register_natives(JNIEnv *e, const char *slash_class, const JNINativeMethod *methods,
                      jint count) {
    if (!e || !slash_class || !methods || count <= 0) return false;
    jclass cls = find_class(e, slash_class);
    if (!cls) return false;
    if (e->RegisterNatives(cls, methods, count) != 0) {
        clear_exception(e);
        return false;
    }
    return true;
}

} // namespace jni
