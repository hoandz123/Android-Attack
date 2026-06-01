#include "mod_ui_internal.hpp"

#include <JNIHelper/JNIHelper.hpp>
#include <android/input.h>
#include <android/log.h>
#include <imgui.h>
#include <jni.h>
#include <string>
#include <vector>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModUiKey", __VA_ARGS__)

namespace modui {

ImGuiKey android_key_to_imgui(int key_code);

namespace {

jclass g_kb_cls;
jmethodID g_sync_ime;

struct KeyEvt {
    int key_code, action, meta, unicode;
};

struct ReplaceEvt {
    int delete_chars;
    std::string insert;
};

std::vector<KeyEvt> g_keys;
std::vector<std::string> g_text;
std::vector<ReplaceEvt> g_replace;

void backspaces(ImGuiIO &io, int n) {
    for (int i = 0; i < n; ++i) {
        io.AddKeyEvent(ImGuiKey_Backspace, true);
        io.AddKeyEvent(ImGuiKey_Backspace, false);
    }
}

} // namespace

void feed_key(int key_code, int action, int meta, int unicode) {
    g_keys.push_back({key_code, action, meta, unicode});
}

void feed_text_utf8(const char *utf8) {
    if (!utf8 || !*utf8) return;
    g_text.emplace_back(utf8);
}

void feed_replace_tail(int delete_chars, const char *utf8) {
    if (delete_chars < 0) delete_chars = 0;
    ReplaceEvt e;
    e.delete_chars = delete_chars;
    if (utf8 && *utf8) e.insert = utf8;
    g_replace.push_back(std::move(e));
}

void apply_pending_keyboard() {
    std::vector<KeyEvt> keys;
    std::vector<std::string> texts;
    std::vector<ReplaceEvt> reps;
    keys.swap(g_keys);
    texts.swap(g_text);
    reps.swap(g_replace);

    ImGuiIO &io = ImGui::GetIO();
    for (const auto &r : reps) {
        backspaces(io, r.delete_chars);
        if (!r.insert.empty()) io.AddInputCharactersUTF8(r.insert.c_str());
    }
    for (const auto &k : keys) {
        const bool down = k.action == 0;
        io.AddKeyEvent(ImGuiMod_Ctrl, (k.meta & AMETA_CTRL_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (k.meta & AMETA_SHIFT_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (k.meta & AMETA_ALT_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (k.meta & AMETA_META_ON) != 0);
        const ImGuiKey key = android_key_to_imgui(k.key_code);
        if (key != ImGuiKey_None) {
            io.AddKeyEvent(key, down);
        } else if (down && k.unicode > 0 && (k.meta & AMETA_CTRL_ON) == 0) {
            io.AddInputCharacter(static_cast<unsigned int>(k.unicode));
        }
    }
    for (const auto &t : texts) io.AddInputCharactersUTF8(t.c_str());
}

void sync_soft_keyboard(bool want) {
    JNIEnv *env = jni::env();
    if (!env || !g_kb_cls || !g_sync_ime) return;
    env->CallStaticVoidMethod(g_kb_cls, g_sync_ime, want ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        jni::clear_exception(env);
        LOGE("syncIme failed");
    }
}

bool init_keyboard_jni(JNIEnv *env, jclass kb_class) {
    if (g_kb_cls) return true;
    if (!env || !kb_class) return false;
    g_kb_cls = reinterpret_cast<jclass>(env->NewGlobalRef(kb_class));
    g_sync_ime = env->GetStaticMethodID(g_kb_cls, "syncIme", "(Z)V");
    if (!g_sync_ime || env->ExceptionCheck()) {
        jni::clear_exception(env);
        return false;
    }
    return true;
}

} // namespace modui
