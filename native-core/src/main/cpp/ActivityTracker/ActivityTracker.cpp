#include "ActivityTracker.hpp"

#define LOG_TAG OBF("ActivityTracker")
#include <Includes/Logger.h>

#include <JNIHelper/JNIHelper.hpp>
#include <unistd.h>
#include <vector>

namespace activity_tracker {

static std::vector<jobject> g_activities;
static jobject g_current = nullptr;

static void ClearRef(JNIEnv *env, jobject &ref) {
    if (!ref) return;
    env->DeleteGlobalRef(ref);
    ref = nullptr;
}

static void LogActivity(JNIEnv *env, jobject activity, const char *event) {
    if (!activity) return;
    jclass cls = env->GetObjectClass(activity);
    if (!cls || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    jclass class_cls = env->FindClass(OBF("java/lang/Class"));
    if (!class_cls || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    jmethodID get_name = env->GetMethodID(class_cls, OBF("getName"), OBF("()Ljava/lang/String;"));
    if (!get_name) return;
    jstring jname = (jstring)env->CallObjectMethod(cls, get_name);
    if (!jname || env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    const char *name = env->GetStringUTFChars(jname, nullptr);
    LOGI(OBF("%s %s"), event, name ? name : OBF("?"));
    if (name) env->ReleaseStringUTFChars(jname, name);
}

void OnActivityResumed(JNIEnv *env, jobject activity) {
    if (!activity) return;
    LogActivity(env, activity, OBF("resumed"));
    ClearRef(env, g_current);
    g_current = env->NewGlobalRef(activity);

    for (jobject a : g_activities) {
        if (env->IsSameObject(a, activity)) return;
    }
    g_activities.push_back(env->NewGlobalRef(activity));
}

void OnActivityPaused(JNIEnv *env, jobject activity) {
    if (!activity) return;
    LogActivity(env, activity, OBF("paused"));
    if (g_current && env->IsSameObject(g_current, activity)) ClearRef(env, g_current);
}

void OnActivityDestroyed(JNIEnv *env, jobject activity) {
    if (!activity) return;
    LogActivity(env, activity, OBF("destroyed"));
    if (g_current && env->IsSameObject(g_current, activity)) ClearRef(env, g_current);

    for (auto it = g_activities.begin(); it != g_activities.end(); ++it) {
        if (env->IsSameObject(*it, activity)) {
            env->DeleteGlobalRef(*it);
            g_activities.erase(it);
            break;
        }
    }
}

static void JNICALL BridgeResumed(JNIEnv *env, jclass, jobject activity) {
    if (env->ExceptionCheck()) env->ExceptionClear();
    OnActivityResumed(env, activity);
}

static void JNICALL BridgePaused(JNIEnv *env, jclass, jobject activity) {
    if (env->ExceptionCheck()) env->ExceptionClear();
    OnActivityPaused(env, activity);
}

static void JNICALL BridgeDestroyed(JNIEnv *env, jclass, jobject activity) {
    if (env->ExceptionCheck()) env->ExceptionClear();
    OnActivityDestroyed(env, activity);
}

static bool RegisterNatives(JNIEnv *env) {
    JNINativeMethod methods[] = {
        {"nativeOnResumed", "(Landroid/app/Activity;)V", (void *)BridgeResumed},
        {"nativeOnPaused", "(Landroid/app/Activity;)V", (void *)BridgePaused},
        {"nativeOnDestroyed", "(Landroid/app/Activity;)V", (void *)BridgeDestroyed},
    };
    if (!jni::RegisterNatives(env, OBF("com/android/attack/nativedex/ActivityTrackerBridge"), methods, 3)) {
        LOGE(OBF("RegisterNatives ActivityTrackerBridge failed"));
        return false;
    }
    return true;
}

static bool CallJavaInstallOnce(JNIEnv *env) {
    jclass bridge = jni::FindClass(env, OBF("com/android/attack/nativedex/ActivityTrackerBridge"));
    if (!bridge) return false;
    jmethodID install = env->GetStaticMethodID(bridge, OBF("install"), OBF("()Z"));
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

static bool CallJavaInstall(JNIEnv *env) {
    for (int i = 0; i < 10; ++i) {
        if (CallJavaInstallOnce(env)) {
            if (i > 0) LOGI(OBF("ActivityTrackerBridge.install ok after retry %d"), i);
            return true;
        }
        usleep(20000);
    }
    LOGE(OBF("ActivityTrackerBridge.install failed — dex loaded? rebuild generateEmbeddedDex"));
    return false;
}

bool Init(JavaVM *vm) {
    if (!vm) return false;
    if (!jni::Inited() && !jni::Init(vm)) return false;
    JNIEnv *env = jni::Env();
    if (!env) return false;

    if (!RegisterNatives(env)) return false;
    if (!CallJavaInstall(env)) return false;

    LOGI(OBF("init ok"));
    return true;
}

std::vector<jobject> GetActivities(JNIEnv *env) {
    std::vector<jobject> out;
    out.reserve(g_activities.size());
    for (jobject a : g_activities) out.push_back(env->NewLocalRef(a));
    return out;
}

jobject CurrentActivity(JNIEnv *env) {
    return g_current ? env->NewLocalRef(g_current) : nullptr;
}

} // namespace activity_tracker
