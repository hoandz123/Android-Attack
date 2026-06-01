#include "mod_ui_internal.hpp"

#include <JNIHelper/JNIHelper.hpp>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>

#define TAG "AttackSurface"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static ANativeWindow *g_window = nullptr;

static void release_window() {
    modui::SetSurface(nullptr);
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

static void JNICALL on_surface_created(JNIEnv *env, jclass, jobject surface) {
    release_window();
    if (!surface) return;
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window || !modui::SetSurface(g_window)) {
        LOGE("surface init failed");
        if (g_window) ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

static void JNICALL on_surface_destroyed(JNIEnv *, jclass) { release_window(); }

static void JNICALL render_frame(JNIEnv *, jclass) {
    if (!modui::HasSurface()) return;
    modui::BeginFrame();
    modui::EndFrame();
}

static void JNICALL on_touch(JNIEnv *, jclass, jint a, jfloat x, jfloat y) {
    modui::FeedTouch(a, x, y);
}

static void JNICALL on_insets(JNIEnv *, jclass, jint l, jint t, jint r, jint b) {
    modui::SetSafeInsets(l, t, r, b);
}

static void JNICALL on_display_metrics(JNIEnv *, jclass, jfloat density) {
    modui::SetDisplayDensity(density);
}

static void JNICALL on_key(JNIEnv *, jclass, jint code, jint action, jint meta, jint unicode) {
    modui::FeedKey(code, action, meta, unicode);
}

static void JNICALL on_text_utf8(JNIEnv *env, jclass, jstring text) {
    if (!text) return;
    const char *utf = env->GetStringUTFChars(text, nullptr);
    if (utf) {
        modui::FeedTextUtf8(utf);
        env->ReleaseStringUTFChars(text, utf);
    }
}

static void JNICALL on_replace_tail(JNIEnv *env, jclass, jint delete_chars, jstring text) {
    const char *utf = nullptr;
    if (text) utf = env->GetStringUTFChars(text, nullptr);
    modui::FeedReplaceTail(delete_chars, utf ? utf : "");
    if (text && utf) env->ReleaseStringUTFChars(text, utf);
}

namespace modui {

bool RegisterSurfaceNatives(JNIEnv *env) {
    static const JNINativeMethod kOverlay[] = {
        {"nativeOnSurfaceCreated", "(Landroid/view/Surface;)V",
         (void *)on_surface_created},
        {"nativeOnSurfaceDestroyed", "()V", (void *)on_surface_destroyed},
        {"nativeRenderFrame", "()V", (void *)render_frame},
    };
    static const JNINativeMethod kTouch[] = {
        {"nativeOnTouch", "(IFF)V", (void *)on_touch},
        {"nativeUpdateInsets", "(IIII)V", (void *)on_insets},
        {"nativeUpdateDisplayMetrics", "(F)V", (void *)on_display_metrics},
    };
    static const JNINativeMethod kKeyboard[] = {
        {"nativeOnKey", "(IIII)V", (void *)on_key},
        {"nativeOnTextUtf8", "(Ljava/lang/String;)V", (void *)on_text_utf8},
        {"nativeOnReplaceTail", "(ILjava/lang/String;)V", (void *)on_replace_tail},
    };
    jclass kb = jni::FindClass(env, "com/android/attack/nativedex/KeyboardInputBridge");
    if (!kb) return false;
    return jni::RegisterNatives(env, "com/android/attack/nativedex/EglOverlay", kOverlay, 3)
        && jni::RegisterNatives(env, "com/android/attack/nativedex/TouchInputBridge", kTouch, 3)
        && jni::RegisterNatives(env, "com/android/attack/nativedex/KeyboardInputBridge", kKeyboard, 3)
        && modui::InitKeyboardJni(env, kb);
}

} // namespace modui
