#include "Internal.hpp"

#define LOG_TAG OBF("ATTACK_ModUiKey")
#include <Includes/Logger.h>

#include <JNIHelper/JNIHelper.hpp>
#include <android/input.h>
#include <imgui.h>
#include <jni.h>
#include <string>
#include <vector>

namespace modui {

ImGuiKey AndroidKeyToImgui(int key_code);

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

constexpr size_t kMaxPendingKeyboard = 512;

void TrimPendingKeyboard() {
    if (g_keys.size() > kMaxPendingKeyboard) g_keys.clear();
    if (g_text.size() > kMaxPendingKeyboard) g_text.clear();
    if (g_replace.size() > kMaxPendingKeyboard) g_replace.clear();
}

} // namespace

void FeedKey(int key_code, int action, int meta, int unicode) {
    g_keys.push_back({key_code, action, meta, unicode});
    TrimPendingKeyboard();
}

void FeedTextUtf8(const char *utf8) {
    if (!utf8 || !*utf8) return;
    g_text.emplace_back(utf8);
    TrimPendingKeyboard();
}

void FeedReplaceTail(int delete_chars, const char *utf8) {
    if (delete_chars < 0) delete_chars = 0;
    ReplaceEvt e;
    e.delete_chars = delete_chars;
    if (utf8 && *utf8) e.insert = utf8;
    g_replace.push_back(std::move(e));
    TrimPendingKeyboard();
}

void ApplyPendingKeyboard() {
    std::vector<KeyEvt> keys;
    std::vector<std::string> texts;
    std::vector<ReplaceEvt> reps;
    keys.swap(g_keys);
    texts.swap(g_text);
    reps.swap(g_replace);

    ImGuiIO &io = ImGui::GetIO();
    for (const auto &r : reps) {
        for (int i = 0; i < r.delete_chars; ++i) {
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            io.AddKeyEvent(ImGuiKey_Backspace, false);
        }
        if (!r.insert.empty()) io.AddInputCharactersUTF8(r.insert.c_str());
    }
    for (const auto &k : keys) {
        const bool down = k.action == 0;
        io.AddKeyEvent(ImGuiMod_Ctrl, (k.meta & AMETA_CTRL_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (k.meta & AMETA_SHIFT_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (k.meta & AMETA_ALT_ON) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (k.meta & AMETA_META_ON) != 0);
        const ImGuiKey key = AndroidKeyToImgui(k.key_code);
        if (key != ImGuiKey_None) {
            io.AddKeyEvent(key, down);
        } else if (down && k.unicode > 0 && (k.meta & AMETA_CTRL_ON) == 0) {
            io.AddInputCharacter(static_cast<unsigned int>(k.unicode));
        }
    }
    for (const auto &t : texts) io.AddInputCharactersUTF8(t.c_str());
}

void SyncSoftKeyboard(bool want) {
    JNIEnv *env = jni::Env();
    if (!env || !g_kb_cls || !g_sync_ime) return;
    env->CallStaticVoidMethod(g_kb_cls, g_sync_ime, want ? JNI_TRUE : JNI_FALSE);
    if (env->ExceptionCheck()) {
        jni::ClearException(env);
        LOGE(OBF("syncIme failed"));
    }
}

bool InitKeyboardJni(JNIEnv *env, jclass kb_class) {
    if (g_kb_cls) return true;
    if (!env || !kb_class) return false;
    g_kb_cls = (jclass)env->NewGlobalRef(kb_class);
    g_sync_ime = env->GetStaticMethodID(g_kb_cls, OBF("syncIme"), OBF("(Z)V"));
    if (!g_sync_ime || env->ExceptionCheck()) {
        jni::ClearException(env);
        return false;
    }
    return true;
}

} // namespace modui
