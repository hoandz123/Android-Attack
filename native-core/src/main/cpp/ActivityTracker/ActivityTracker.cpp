#include "ActivityTracker.hpp"

#include <JNIHelper/JNIHelper.hpp>
#include <android/log.h>
#include <unistd.h>
#include <vector>

#define TAG "ActivityTracker"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace activity_tracker {

static std::vector<jobject> g_activities;
static jobject g_current = nullptr;

static void clear_ref(JNIEnv *env, jobject &ref) {
    if (!ref) return;
    env->DeleteGlobalRef(ref);
    ref = nullptr;
}

static void log_activity(JNIEnv *env, jobject activity, const char *event) {
    if (!activity) return;
    jclass cls = env->GetObjectClass(activity);
    if (!cls || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    jclass class_cls = env->FindClass("java/lang/Class");
    if (!class_cls || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    jmethodID get_name = env->GetMethodID(class_cls, "getName", "()Ljava/lang/String;");
    if (!get_name) return;
    jstring jname = reinterpret_cast<jstring>(env->CallObjectMethod(cls, get_name));
    if (!jname || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    const char *name = env->GetStringUTFChars(jname, nullptr);
    LOGI("%s %s", event, name ? name : "?");
    if (name) env->ReleaseStringUTFChars(jname, name);
}

void on_activity_resumed(JNIEnv *env, jobject activity) {
    if (!activity) return;
    log_activity(env, activity, "resumed");
    clear_ref(env, g_current);
    g_current = env->NewGlobalRef(activity);

    for (jobject a : g_activities) {
        if (env->IsSameObject(a, activity)) return;
    }
    g_activities.push_back(env->NewGlobalRef(activity));
}

void on_activity_paused(JNIEnv *env, jobject activity) {
    if (!activity) return;
    log_activity(env, activity, "paused");
    if (g_current && env->IsSameObject(g_current, activity)) clear_ref(env, g_current);
}

void on_activity_destroyed(JNIEnv *env, jobject activity) {
    if (!activity) return;
    log_activity(env, activity, "destroyed");
    if (g_current && env->IsSameObject(g_current, activity)) clear_ref(env, g_current);

    for (auto it = g_activities.begin(); it != g_activities.end(); ++it) {
        if (env->IsSameObject(*it, activity)) {
            env->DeleteGlobalRef(*it);
            g_activities.erase(it);
            break;
        }
    }
}

static void JNICALL bridge_resumed(JNIEnv *env, jclass, jobject activity) {
    if (env->ExceptionCheck()) env->ExceptionClear();
    on_activity_resumed(env, activity);
}

static void JNICALL bridge_paused(JNIEnv *env, jclass, jobject activity) {
    if (env->ExceptionCheck()) env->ExceptionClear();
    on_activity_paused(env, activity);
}

static void JNICALL bridge_destroyed(JNIEnv *env, jclass, jobject activity) {
    if (env->ExceptionCheck()) env->ExceptionClear();
    on_activity_destroyed(env, activity);
}

static bool register_natives(JNIEnv *env) {
    JNINativeMethod methods[] = {
        {"nativeOnResumed", "(Landroid/app/Activity;)V", reinterpret_cast<void *>(bridge_resumed)},
        {"nativeOnPaused", "(Landroid/app/Activity;)V", reinterpret_cast<void *>(bridge_paused)},
        {"nativeOnDestroyed", "(Landroid/app/Activity;)V", reinterpret_cast<void *>(bridge_destroyed)},
    };
    if (!jni::register_natives(env, "com/android/attack/nativedex/ActivityTrackerBridge", methods, 3)) {
        LOGE("RegisterNatives ActivityTrackerBridge failed");
        return false;
    }
    return true;
}

static bool call_java_install_once(JNIEnv *env) {
    jclass bridge = jni::find_class(env, "com/android/attack/nativedex/ActivityTrackerBridge");
    if (!bridge) return false;
    jmethodID install = env->GetStaticMethodID(bridge, "install", "()Z");
    if (!install || env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    const jboolean ok = env->CallStaticBooleanMethod(bridge, install);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }
    return ok == JNI_TRUE;
}

static bool call_java_install(JNIEnv *env) {
    for (int i = 0; i < 10; ++i) {
        if (call_java_install_once(env)) {
            if (i > 0) LOGI("ActivityTrackerBridge.install ok after retry %d", i);
            return true;
        }
        usleep(20000);
    }
    LOGE("ActivityTrackerBridge.install failed — dex loaded? rebuild generateEmbeddedDex");
    return false;
}

bool init(JavaVM *vm) {
    if (!vm) return false;
    if (!jni::inited() && !jni::init(vm)) return false;
    JNIEnv *env = jni::env();
    if (!env) return false;

    if (!register_natives(env)) return false;
    if (!call_java_install(env)) return false;

    LOGI("init ok");
    return true;
}

std::vector<jobject> activities(JNIEnv *env) {
    std::vector<jobject> out;
    out.reserve(g_activities.size());
    for (jobject a : g_activities) out.push_back(env->NewLocalRef(a));
    return out;
}

jobject current_activity(JNIEnv *env) {
    return g_current ? env->NewLocalRef(g_current) : nullptr;
}

} // namespace activity_tracker
