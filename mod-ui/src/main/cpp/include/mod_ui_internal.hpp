#pragma once

#include <jni.h>

#include "mod_ui.hpp"

struct ANativeWindow;

namespace modui {

bool register_surface_natives(JNIEnv *env);

void shutdown();

bool set_surface(ANativeWindow *window);
bool has_surface();
void begin_frame();
void end_frame();

void set_menu_visible(bool visible);
bool menu_visible();

void feed_touch(int action, float x, float y);
void set_safe_insets(float left, float top, float right, float bottom);
void set_display_density(float density);
void apply_pending_touch();
void apply_safe_area_style();

void feed_key(int key_code, int action, int meta, int unicode);
void feed_text_utf8(const char *utf8);
void feed_replace_tail(int delete_chars, const char *utf8);
void apply_pending_keyboard();
void sync_soft_keyboard(bool want);
bool init_keyboard_jni(JNIEnv *env, jclass keyboard_bridge_class);

void draw_menu_shell(const AppUi &ui);

void setup_ui_fonts();

} // namespace modui
