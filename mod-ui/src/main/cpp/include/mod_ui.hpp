#pragma once

struct ANativeWindow;

namespace modui {

// App plugin draws widgets inside the shell window (tabs, toggles, etc.).
using DrawMenuFn = void (*)();

struct AppUi {
    DrawMenuFn draw_menu = nullptr;
};

bool init();
void shutdown();

void set_app_ui(const AppUi &ui);
const AppUi &app_ui();

// GLES surface from Java/SurfaceView — required before frames render.
bool set_surface(ANativeWindow *window);
bool has_surface();

void set_menu_visible(bool visible);
bool menu_visible();

void begin_frame();
void end_frame();

} // namespace modui
