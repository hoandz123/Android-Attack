#pragma once

namespace modui {

/** Plugin đăng ký menu ImGui (gọi từ :app sau dex load). */
using DrawMenuFn = void (*)();

struct AppUi {
    DrawMenuFn draw_menu = nullptr;
};

/** ImGui context + JNI natives (EglOverlay / touch / keyboard). Cần jni::init + dex trước. */
bool init();

void set_app_ui(const AppUi &ui);
const AppUi &app_ui();

} // namespace modui
