#include "UI/OverlaySurface.h"
#include "Core/Bootstrap.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include <android/native_window_jni.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace overlaySurface {

namespace {

std::mutex g_mutex;
std::condition_variable g_cv;
ANativeWindow *g_window = nullptr;
int g_width = 0;
int g_height = 0;
bool g_ready = false;
bool g_destroyed = false;

void releaseWindowLocked(JNIEnv *env) {
    if (g_window != nullptr) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
    g_ready = false;
    g_destroyed = true;
    (void) env;
}

}

bool create() {
    JNIEnv *env = bootstrap::getEnv();
    if (!env) { LOGE(OBFUSCATE("overlaySurface::create: no JNIEnv")); return false; }
    jobject activity = bootstrap::getActivity(env);
    if (!activity) { LOGE(OBFUSCATE("overlaySurface::create: no activity")); return false; }
    bootstrap::post([activity]() {
        JNIEnv *uiEnv = bootstrap::getEnv();
        if (!uiEnv) { LOGE(OBFUSCATE("overlaySurface::create: ui thread no env")); return; }
        jclass bridgeCls = dexInject::loadBridgeClass(uiEnv, OBFUSCATE("com.android.attack.NativeBridge"));
        jmethodID method = uiEnv->GetStaticMethodID(bridgeCls, OBFUSCATE("createOverlay"), OBFUSCATE("(Landroid/app/Activity;)V"));
        uiEnv->CallStaticVoidMethod(bridgeCls, method, activity);
        if (bootstrap::checkException(uiEnv, OBFUSCATE("overlaySurface::create: createOverlay call"))) LOGE(OBFUSCATE("overlaySurface::create: createOverlay failed"));
        else LOGI(OBFUSCATE("overlaySurface::create: overlay posted"));
    });
    return true;
}

bool waitReady(int timeoutMs) {
    std::unique_lock<std::mutex> lock(g_mutex);
    return g_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), []() { return g_ready; });
}

ANativeWindow *getWindow() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_window;
}

int getWidth() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_width;
}

int getHeight() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_height;
}

void onSurfaceReady(JNIEnv *env, jclass clazz, jobject surface, jint width, jint height) {
    (void) clazz;
    std::lock_guard<std::mutex> lock(g_mutex);
    releaseWindowLocked(env);
    g_destroyed = false;
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window) { LOGE(OBFUSCATE("overlaySurface::onSurfaceReady: ANativeWindow_fromSurface failed")); return; }
    g_width = (int) width;
    g_height = (int) height;
    g_ready = true;
    LOGI(OBFUSCATE("overlaySurface::onSurfaceReady: %dx%d"), g_width, g_height);
    g_cv.notify_all();
}

void onSurfaceChanged(JNIEnv *env, jclass clazz, jobject surface, jint width, jint height) {
    (void) clazz;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_window != nullptr) ANativeWindow_release(g_window);
    g_window = ANativeWindow_fromSurface(env, surface);
    g_width = (int) width;
    g_height = (int) height;
    g_ready = g_window != nullptr;
    LOGI(OBFUSCATE("overlaySurface::onSurfaceChanged: %dx%d"), g_width, g_height);
    g_cv.notify_all();
}

void onSurfaceDestroyed(JNIEnv *env, jclass clazz) {
    (void) clazz;
    std::lock_guard<std::mutex> lock(g_mutex);
    releaseWindowLocked(env);
    LOGI(OBFUSCATE("overlaySurface::onSurfaceDestroyed"));
    g_cv.notify_all();
}

}
