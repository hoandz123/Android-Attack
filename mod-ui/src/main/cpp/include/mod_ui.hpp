#pragma once

namespace modui {

using DrawTabFn = void (*)();

struct MenuTab {
    const char *id = nullptr;
    const char *label = nullptr;
    DrawTabFn draw = nullptr;
};

struct AppUi {
    static constexpr int kMaxTabs = 16;

    const char *window_title = nullptr;

    MenuTab tabs[kMaxTabs]{};
    int tab_count = 0;

    bool set_window_title(const char *title) {
        if (!title || !title[0]) return false;
        window_title = title;
        return true;
    }

    bool add_tab(const char *id, const char *label, DrawTabFn draw) {
        if (!id || !label || !draw || tab_count >= kMaxTabs) return false;
        tabs[tab_count++] = MenuTab{id, label, draw};
        return true;
    }
};

bool init();
void set_app_ui(const AppUi &ui);
const AppUi &app_ui();
void set_menu_visible(bool visible);
bool menu_visible();
void set_menu_expanded(bool expanded);
bool menu_expanded();

} // namespace modui
