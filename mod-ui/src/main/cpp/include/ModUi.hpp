#pragma once

namespace modui {

using DrawTabFn = void (*)();
using OverlayDrawFn = void (*)();

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

bool Init();
bool RegisterOverlayDraw(OverlayDrawFn draw);
void SetAppUi(const AppUi &ui);
const AppUi &GetAppUi();
void SetMenuVisible(bool visible);
bool MenuVisible();
void SetMenuExpanded(bool expanded);
bool MenuExpanded();

} // namespace modui
