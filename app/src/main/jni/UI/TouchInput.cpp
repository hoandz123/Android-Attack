#include "UI/TouchInput.h"

#include "Core/Bootstrap.h"

#include "Includes/Logger.h"

#include "Includes/obfuscate.h"

#include "imgui.h"

#include <android/input.h>

#include <cmath>

#include <mutex>

namespace touchInput {

namespace {

std::mutex g_mutex;

bool g_mouseDown = false;

float g_mouseX = 0.0f;

float g_mouseY = 0.0f;

bool g_pendingPos = false;

bool g_pendingDown = false;

bool g_pendingUp = false;

bool g_wantCaptureMouse = false;

float g_capX = 0.0f;

float g_capY = 0.0f;

float g_capW = 0.0f;

float g_capH = 0.0f;

bool g_hasCaptureRect = false;

bool g_inputRectSynced = false;

bool g_syncDirty = false;

int g_lastSyncX = -1;

int g_lastSyncY = -1;

int g_lastSyncW = -1;

int g_lastSyncH = -1;

constexpr float kRectEpsilon = 0.5f;

bool hitCaptureRectLocked(float x, float y) {

    if (!g_hasCaptureRect) return false;

    return x >= g_capX && x <= (g_capX + g_capW) && y >= g_capY && y <= (g_capY + g_capH);

}

bool captureRectChangedLocked(int x, int y, int w, int h) {

    return x != g_lastSyncX || y != g_lastSyncY || w != g_lastSyncW || h != g_lastSyncH;

}

void postUpdateInputRectLocked(int x, int y, int w, int h) {

    g_lastSyncX = x;

    g_lastSyncY = y;

    g_lastSyncW = w;

    g_lastSyncH = h;

    g_inputRectSynced = true;

    g_syncDirty = false;

    bootstrap::post([x, y, w, h]() {

        JNIEnv *uiEnv = bootstrap::getEnv();

        if (!uiEnv) return;

        jclass bridgeCls = dexInject::loadBridgeClass(uiEnv, OBFUSCATE("com.android.attack.NativeBridge"));

        if (!bridgeCls) return;

        jmethodID method = uiEnv->GetStaticMethodID(bridgeCls, OBFUSCATE("updateInputRect"), OBFUSCATE("(IIII)V"));

        if (!method) { bootstrap::checkException(uiEnv, OBFUSCATE("touchInput::updateInputRect")); return; }

        uiEnv->CallStaticVoidMethod(bridgeCls, method, (jint) x, (jint) y, (jint) w, (jint) h);

        bootstrap::checkException(uiEnv, OBFUSCATE("touchInput::updateInputRect call"));

    });

}

void postHideInputRectLocked() {

    g_lastSyncX = 0;

    g_lastSyncY = 0;

    g_lastSyncW = 0;

    g_lastSyncH = 0;

    g_inputRectSynced = false;

    g_syncDirty = false;

    bootstrap::post([]() {

        JNIEnv *uiEnv = bootstrap::getEnv();

        if (!uiEnv) return;

        jclass bridgeCls = dexInject::loadBridgeClass(uiEnv, OBFUSCATE("com.android.attack.NativeBridge"));

        if (!bridgeCls) return;

        jmethodID method = uiEnv->GetStaticMethodID(bridgeCls, OBFUSCATE("hideInputRect"), OBFUSCATE("()V"));

        if (!method) { bootstrap::checkException(uiEnv, OBFUSCATE("touchInput::hideInputRect")); return; }

        uiEnv->CallStaticVoidMethod(bridgeCls, method);

        bootstrap::checkException(uiEnv, OBFUSCATE("touchInput::hideInputRect call"));

    });

}

void syncInputWindowLocked(bool force) {

    if (!force && g_mouseDown) return;

    if (!g_hasCaptureRect || g_capW <= kRectEpsilon || g_capH <= kRectEpsilon) {

        if (g_inputRectSynced || g_syncDirty) postHideInputRectLocked();

        return;

    }

    int ix = (int) g_capX;

    int iy = (int) g_capY;

    int iw = (int) g_capW;

    int ih = (int) g_capH;

    if (!force && !g_syncDirty && !captureRectChangedLocked(ix, iy, iw, ih)) return;

    postUpdateInputRectLocked(ix, iy, iw, ih);

}

void resetTouchStateLocked() {

    g_mouseDown = false;

    g_pendingDown = false;

    g_pendingUp = false;

    g_pendingPos = false;

}

}

void setCaptureRect(float x, float y, float w, float h) {

    std::lock_guard<std::mutex> lock(g_mutex);

    g_capX = x;

    g_capY = y;

    g_capW = w;

    g_capH = h;

    g_hasCaptureRect = (w > kRectEpsilon && h > kRectEpsilon);

    g_syncDirty = true;

}

void clearCaptureRect() {

    std::lock_guard<std::mutex> lock(g_mutex);

    g_hasCaptureRect = false;

    g_capW = 0.0f;

    g_capH = 0.0f;

    g_syncDirty = true;

    resetTouchStateLocked();

}

void applyPendingTouch() {

    std::lock_guard<std::mutex> lock(g_mutex);

    ImGuiIO &io = ImGui::GetIO();

    if (g_pendingPos) { io.AddMousePosEvent(g_mouseX, g_mouseY); g_pendingPos = false; }

    if (g_pendingDown) { io.AddMouseButtonEvent(0, true); g_pendingDown = false; g_mouseDown = true; }

    if (g_pendingUp) { io.AddMouseButtonEvent(0, false); g_pendingUp = false; g_mouseDown = false; }

    g_wantCaptureMouse = io.WantCaptureMouse;

}

void syncInputWindowAfterFrame() {

    std::lock_guard<std::mutex> lock(g_mutex);

    syncInputWindowLocked(false);

}

bool wantCaptureMouse() {

    std::lock_guard<std::mutex> lock(g_mutex);

    return g_wantCaptureMouse;

}

jboolean onNativeTouch(JNIEnv *env, jclass clazz, jlong id, jint action, jfloat rawX, jfloat rawY) {

    (void) env;

    (void) clazz;

    (void) id;

    std::lock_guard<std::mutex> lock(g_mutex);

    g_mouseX = rawX;

    g_mouseY = rawY;

    g_pendingPos = true;

    if (action == AMOTION_EVENT_ACTION_DOWN) g_pendingDown = true;

    else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {

        g_pendingUp = true;

        g_mouseDown = false;

        g_syncDirty = true;

        syncInputWindowLocked(true);

    }

    bool capture = hitCaptureRectLocked(rawX, rawY);

    if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) return capture ? JNI_TRUE : JNI_FALSE;

    if (action == AMOTION_EVENT_ACTION_DOWN) return capture ? JNI_TRUE : JNI_FALSE;

    return (capture && (g_wantCaptureMouse || g_mouseDown || g_pendingDown)) ? JNI_TRUE : JNI_FALSE;

}

}
