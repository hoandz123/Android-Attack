#pragma once

#include <jni.h>

#include "ModUi.hpp"

struct ANativeWindow;

namespace modui {

bool RegisterSurfaceNatives(JNIEnv *env);

void Shutdown();

bool SetSurface(ANativeWindow *window);
bool HasSurface();
void BeginFrame();
void EndFrame();

void SetMenuVisible(bool visible);
bool MenuVisible();
void SetMenuExpanded(bool expanded);
bool MenuExpanded();

void FeedTouch(int action, float x, float y);
void SetSafeInsets(float left, float top, float right, float bottom);
void SetDisplayDensity(float density);
void ApplyPendingTouch();
void ApplySafeAreaStyle();

void FeedKey(int key_code, int action, int meta, int unicode);
void FeedTextUtf8(const char *utf8);
void FeedReplaceTail(int delete_chars, const char *utf8);
void ApplyPendingKeyboard();
void SyncSoftKeyboard(bool want);
bool InitKeyboardJni(JNIEnv *env, jclass keyboard_bridge_class);

void DrawMenuShell(const AppUi &ui);

void SetupUiFonts();

} // namespace modui
