#include "mod_ui_internal.hpp"

#include <JNIHelper/JNIHelper.hpp>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <jni.h>

#define TAG "AttackSurface"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static ANativeWindow *g_window = nullptr;

static void release_window() {
    modui::set_surface(nullptr);
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

static void JNICALL on_surface_created(JNIEnv *env, jclass, jobject surface) {
    release_window();
    if (!surface) return;
    g_window = ANativeWindow_fromSurface(env, surface);
    if (!g_window || !modui::set_surface(g_window)) {
        LOGE("surface init failed");
        if (g_window) ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

static void JNICALL on_surface_destroyed(JNIEnv *, jclass) { release_window(); }

static void JNICALL render_frame(JNIEnv *, jclass) {
    if (!modui::has_surface()) return;
    modui::begin_frame();
    modui::end_frame();
}

static void JNICALL on_touch(JNIEnv *, jclass, jint a, jfloat x, jfloat y) {
    modui::feed_touch(a, x, y);
}

static void JNICALL on_insets(JNIEnv *, jclass, jint l, jint t, jint r, jint b) {
    modui::set_safe_insets(l, t, r, b);
}

static void JNICALL on_display_metrics(JNIEnv *, jclass, jfloat density) {
    modui::set_display_density(density);
}

static void JNICALL on_key(JNIEnv *, jclass, jint code, jint action, jint meta, jint unicode) {
    modui::feed_key(code, action, meta, unicode);
}

static void JNICALL on_text_utf8(JNIEnv *env, jclass, jstring text) {
    if (!text) return;
    const char *utf = env->GetStringUTFChars(text, nullptr);
    if (utf) {
        modui::feed_text_utf8(utf);
        env->ReleaseStringUTFChars(text, utf);
    }
}

static void JNICALL on_replace_tail(JNIEnv *env, jclass, jint delete_chars, jstring text) {
    const char *utf = nullptr;
    if (text) utf = env->GetStringUTFChars(text, nullptr);
    modui::feed_replace_tail(delete_chars, utf ? utf : "");
    if (text && utf) env->ReleaseStringUTFChars(text, utf);
}

namespace modui {

bool register_surface_natives(JNIEnv *env) {
    static const JNINativeMethod kOverlay[] = {
        {"nativeOnSurfaceCreated", "(Landroid/view/Surface;)V",
         reinterpret_cast<void *>(on_surface_created)},
        {"nativeOnSurfaceDestroyed", "()V", reinterpret_cast<void *>(on_surface_destroyed)},
        {"nativeRenderFrame", "()V", reinterpret_cast<void *>(render_frame)},
    };
    static const JNINativeMethod kTouch[] = {
        {"nativeOnTouch", "(IFF)V", reinterpret_cast<void *>(on_touch)},
        {"nativeUpdateInsets", "(IIII)V", reinterpret_cast<void *>(on_insets)},
        {"nativeUpdateDisplayMetrics", "(F)V", reinterpret_cast<void *>(on_display_metrics)},
    };
    static const JNINativeMethod kKeyboard[] = {
        {"nativeOnKey", "(IIII)V", reinterpret_cast<void *>(on_key)},
        {"nativeOnTextUtf8", "(Ljava/lang/String;)V", reinterpret_cast<void *>(on_text_utf8)},
        {"nativeOnReplaceTail", "(ILjava/lang/String;)V", reinterpret_cast<void *>(on_replace_tail)},
    };
    jclass kb = jni::find_class(env, "com/android/attack/nativedex/KeyboardInputBridge");
    if (!kb) return false;
    return jni::register_natives(env, "com/android/attack/nativedex/EglOverlay", kOverlay, 3)
        && jni::register_natives(env, "com/android/attack/nativedex/TouchInputBridge", kTouch, 3)
        && jni::register_natives(env, "com/android/attack/nativedex/KeyboardInputBridge", kKeyboard, 3)
        && modui::init_keyboard_jni(env, kb);
}

} // namespace modui
