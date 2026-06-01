#include "Internal.hpp"

#define LOG_TAG "AttackSurface"
#include <Includes/Logger.h>

#include <JNIHelper/JNIHelper.hpp>
#include <android/native_window_jni.h>
#include <jni.h>

static ANativeWindow *g_window = nullptr;

static void ReleaseWindow() {
    modui::SetSurface(nullptr);
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

static void JNICALL OnSurfaceCreated(JNIEnv *env, jclass, jobject surface) {
    ReleaseWindow();
    if (!surface) return;
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window || !modui::SetSurface(g_window)) {
        LOGE("surface init failed");
        if (g_window) ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

static void JNICALL OnSurfaceDestroyed(JNIEnv *, jclass) { ReleaseWindow(); }

static void JNICALL RenderFrame(JNIEnv *, jclass) {
    if (!modui::HasSurface()) return;
    modui::BeginFrame();
    modui::EndFrame();
}

static void JNICALL OnTouch(JNIEnv *, jclass, jint a, jfloat x, jfloat y) {
    modui::FeedTouch(a, x, y);
}

static void JNICALL OnInsets(JNIEnv *, jclass, jint l, jint t, jint r, jint b) {
    modui::SetSafeInsets(l, t, r, b);
}

static void JNICALL OnDisplayMetrics(JNIEnv *, jclass, jfloat density) {
    modui::SetDisplayDensity(density);
}

static void JNICALL OnKey(JNIEnv *, jclass, jint code, jint action, jint meta, jint unicode) {
    modui::FeedKey(code, action, meta, unicode);
}

static void JNICALL OnTextUtf8(JNIEnv *env, jclass, jstring text) {
    if (!text) return;
    const char *utf = env->GetStringUTFChars(text, nullptr);
    if (utf) {
        modui::FeedTextUtf8(utf);
        env->ReleaseStringUTFChars(text, utf);
    }
}

static void JNICALL OnReplaceTail(JNIEnv *env, jclass, jint delete_chars, jstring text) {
    const char *utf = nullptr;
    if (text) utf = env->GetStringUTFChars(text, nullptr);
    modui::FeedReplaceTail(delete_chars, utf ? utf : "");
    if (text && utf) env->ReleaseStringUTFChars(text, utf);
}

namespace modui {

bool RegisterSurfaceNatives(JNIEnv *env) {
    static const JNINativeMethod kOverlay[] = {
        {"nativeOnSurfaceCreated", "(Landroid/view/Surface;)V",
         (void *)OnSurfaceCreated},
        {"nativeOnSurfaceDestroyed", "()V", (void *)OnSurfaceDestroyed},
        {"nativeRenderFrame", "()V", (void *)RenderFrame},
    };
    static const JNINativeMethod kTouch[] = {
        {"nativeOnTouch", "(IFF)V", (void *)OnTouch},
        {"nativeUpdateInsets", "(IIII)V", (void *)OnInsets},
        {"nativeUpdateDisplayMetrics", "(F)V", (void *)OnDisplayMetrics},
    };
    static const JNINativeMethod kKeyboard[] = {
        {"nativeOnKey", "(IIII)V", (void *)OnKey},
        {"nativeOnTextUtf8", "(Ljava/lang/String;)V", (void *)OnTextUtf8},
        {"nativeOnReplaceTail", "(ILjava/lang/String;)V", (void *)OnReplaceTail},
    };
    jclass kb = jni::FindClass(env, "com/android/attack/nativedex/KeyboardInputBridge");
    if (!kb) return false;
    return jni::RegisterNatives(env, "com/android/attack/nativedex/EglOverlay", kOverlay, 3)
        && jni::RegisterNatives(env, "com/android/attack/nativedex/TouchInputBridge", kTouch, 3)
        && jni::RegisterNatives(env, "com/android/attack/nativedex/KeyboardInputBridge", kKeyboard, 3)
        && modui::InitKeyboardJni(env, kb);
}

} // namespace modui
