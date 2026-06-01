#include "mod_ui.hpp"
#include "mod_ui_internal.hpp"
#include "mod_ui_layout.hpp"
#include "mod_ui_icon.hpp"
#include <imgui.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <thread>

namespace modui {

namespace {

constexpr float kSidebarWidthFrac = 0.30f;
constexpr float kSidebarMinDp = 96.f;
constexpr float kTabRowDp = 48.f;
constexpr float kTabGapDp = 5.f;
constexpr float kTabRoundDp = 10.f;
constexpr float kTabPadXDp = 12.f;
constexpr float kRibbonWidthDp = 4.f;
constexpr float kRibbonInsetVDp = 6.f;
constexpr float kPanelGapDp = 6.f;
constexpr float kFabDp = 56.f;
constexpr float kFabMarginDp = 28.f;
constexpr float kFabEdgePadDp = 14.f;
constexpr int kMaxTabAnim = 16;
constexpr float kAnimSpeed = 16.f;

constexpr const char *kDefaultWindowTitle = "Mod Menu##modui_shell";
constexpr const char *kFabWindowId = "##modui_fab";

struct SidebarTabAnimState {
    float blend[kMaxTabAnim]{};
};

SidebarTabAnimState g_tab_anim;

ImVec2 fab_default_pos(float fab_sz) {
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 safe = ImGui::GetStyle().DisplaySafeAreaPadding;
    const float margin = dp_to_px(kFabMarginDp);
    return ImVec2(vp->WorkPos.x + margin + safe.x, vp->WorkPos.y + margin + safe.y);
}

void clamp_fab_pos(ImVec2 &pos, float fab_sz) {
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 safe = ImGui::GetStyle().DisplaySafeAreaPadding;
    const float pad = dp_to_px(kFabEdgePadDp);
    const float x0 = vp->WorkPos.x + safe.x + pad;
    const float y0 = vp->WorkPos.y + safe.y + pad;
    const float x1 = vp->WorkPos.x + vp->WorkSize.x - fab_sz - safe.x - pad;
    const float y1 = vp->WorkPos.y + vp->WorkSize.y - fab_sz - safe.y - pad;
    pos.x = std::clamp(pos.x, x0, std::max(x0, x1));
    pos.y = std::clamp(pos.y, y0, std::max(y0, y1));
}

ImVec4 panel_bg_sidebar() { return ImVec4(0.11f, 0.12f, 0.145f, 1.f); }
ImVec4 panel_bg_content() { return ImVec4(0.13f, 0.14f, 0.17f, 1.f); }

float smooth_toward(float current, float target, float dt) {
    const float k = 1.f - std::exp(-kAnimSpeed * dt);
    return current + (target - current) * k;
}

ImVec4 lerp_color(const ImVec4 &a, const ImVec4 &b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                  a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

ImU32 color_to_u32(const ImVec4 &c) {
    return ImGui::ColorConvertFloat4ToU32(c);
}

float sidebar_width(float content_w) {
    const float min_w = dp_to_px(kSidebarMinDp);
    const float frac_w = content_w * kSidebarWidthFrac;
    return std::max(min_w, frac_w);
}

const char *resolve_window_title(const AppUi &ui) {
    if (ui.window_title && ui.window_title[0]) return ui.window_title;
    return kDefaultWindowTitle;
}

bool draw_sidebar_tab_ribbon(int index, const char *label, bool selected, float row_h,
                             float width) {
    if (index < 0 || index >= kMaxTabAnim) return false;

    const float dt = ImGui::GetIO().DeltaTime;
    g_tab_anim.blend[index] = smooth_toward(g_tab_anim.blend[index], selected ? 1.f : 0.f, dt);

    const float round = dp_to_px(kTabRoundDp);
    const float pad_x = dp_to_px(kTabPadXDp);
    const float ribbon_w = dp_to_px(kRibbonWidthDp);
    const float ribbon_inset = dp_to_px(kRibbonInsetVDp);
    const ImVec2 size(width, row_h);

    ImGui::PushID(index);
    const bool pressed = ImGui::InvisibleButton("##tab", size);
    const bool held = ImGui::IsItemActive();
    ImGui::PopID();

    const ImVec2 pos = ImGui::GetItemRectMin();
    const ImVec2 pos_max = ImGui::GetItemRectMax();
    ImDrawList *dl = ImGui::GetWindowDrawList();

    const float press_scale = held ? 0.985f : 1.f;
    const ImVec2 shrink((1.f - press_scale) * size.x * 0.5f,
                        (1.f - press_scale) * size.y * 0.5f);
    const ImVec2 p0(pos.x + shrink.x, pos.y + shrink.y);
    const ImVec2 p1(pos_max.x - shrink.x, pos_max.y - shrink.y);

    const float sel = g_tab_anim.blend[index];

    if (sel > 0.02f) {
        const ImU32 left = IM_COL32(0, 0, 0, 0);
        const ImU32 right = color_to_u32(ImVec4(0.05f, 0.48f, 0.86f, 0.15f * sel));
        dl->AddRectFilledMultiColor(p0, p1, left, right, right, left);

        const float aw = ribbon_w * sel;
        const ImVec2 r0(p1.x - aw, p0.y + ribbon_inset);
        const ImVec2 r1(p1.x, p1.y - ribbon_inset);
        dl->AddRectFilled(r0, r1, color_to_u32(ImVec4(0.12f, 0.58f, 0.95f, sel)), 3.f,
                          ImDrawFlags_RoundCornersRight);
    }

    const char *text = label ? label : "";
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const float text_x = p0.x + pad_x;
    const float text_y = p0.y + (p1.y - p0.y - ts.y) * 0.5f;
    const ImVec4 text_idle(0.72f, 0.76f, 0.84f, 1.f);
    const ImVec4 text_sel(1.f, 1.f, 1.f, 1.f);
    const ImVec4 text_c = lerp_color(text_idle, text_sel, sel);
    dl->AddText(ImVec2(text_x, text_y), color_to_u32(text_c), text);

    return pressed;
}

void draw_sidebar(const AppUi &ui, int &selected, ImVec2 panel_size) {
    const float row_h = dp_to_px(kTabRowDp);
    const float gap = dp_to_px(kPanelGapDp);
    const float tab_gap = dp_to_px(kTabGapDp);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg_sidebar());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(gap, gap));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, tab_gap));
    if (ImGui::BeginChild("modui_sidebar", panel_size, ImGuiChildFlags_Borders)) {
        const float tab_w = ImGui::GetContentRegionAvail().x;

        for (int i = 0; i < ui.tab_count; ++i) {
            const MenuTab &tab = ui.tabs[i];
            const char *label = tab.label ? tab.label : "";
            const bool is_sel = (i == selected);
            if (draw_sidebar_tab_ribbon(i, label, is_sel, row_h, tab_w)) selected = i;
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void draw_content_panel(const AppUi &ui, int selected, ImVec2 panel_size) {
    const float gap = dp_to_px(kPanelGapDp);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, panel_bg_content());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(gap * 1.25f, gap * 1.25f));
    if (ImGui::BeginChild("modui_content", panel_size, ImGuiChildFlags_Borders)) {
        if (selected >= 0 && selected < ui.tab_count) {
            const MenuTab &tab = ui.tabs[selected];
            if (tab.draw) tab.draw();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void draw_menu_fab(const AppUi &, ImTextureID icon_tex) {
    const float fab = dp_to_px(kFabDp);
    const ImVec2 fab_sz(fab, fab);
    const bool has_tex = icon_tex != (ImTextureID) 0;

    ImGui::SetNextWindowPos(fab_default_pos(fab), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(fab_sz, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse
                             | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
    if (has_tex) flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fab * 0.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    if (has_tex) {
        ImGui::SetNextWindowBgAlpha(0.f);
    } else {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 1.f));
    }
    if (!ImGui::Begin(kFabWindowId, nullptr, flags)) {
        if (!has_tex) ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        return;
    }
    if (!has_tex) ImGui::PopStyleColor();

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + fab_sz.x, p0.y + fab_sz.y);
    if (has_tex) {
        ImGui::GetWindowDrawList()->AddImageRounded(
            icon_tex, p0, p1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, fab * 0.5f,
            ImDrawFlags_RoundCornersAll);
    } else {
        ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255), fab * 0.5f);
    }
    ImGui::Dummy(fab_sz);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(0)) {
        const ImVec2 d = ImGui::GetMouseDragDelta(0);
        const float th = ImGui::GetIO().MouseDragThreshold;
        if (d.x * d.x + d.y * d.y <= th * th) set_menu_expanded(true);
    }

    ImVec2 pos = ImGui::GetWindowPos();
    clamp_fab_pos(pos, fab);
    const ImVec2 cur = ImGui::GetWindowPos();
    if (pos.x != cur.x || pos.y != cur.y) ImGui::SetWindowPos(pos);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace

static ImTextureID g_fab_icon = (ImTextureID) 0;
static std::atomic<bool> g_fab_downloading{false};
static std::atomic<bool> g_fab_file_ready{false};
static constexpr const char *kFabIconUrl = "https://files.catbox.moe/vpu7cc.png";
static constexpr const char *kFabIconPath = "/data/user/0/com.android.attack/fab.png";

void draw_menu_shell(const AppUi &ui) {
    if (!menu_visible()) return;

    if (!g_fab_icon) {
        if (!g_fab_file_ready.load(std::memory_order_acquire)) {
            bool expected = false;
            if (g_fab_downloading.compare_exchange_strong(expected, true)) {
                std::thread([] {
                    const bool ok = menu_icon_download(kFabIconUrl, kFabIconPath);
                    g_fab_file_ready.store(ok, std::memory_order_release);
                }).detach();
            }
        } else {
            g_fab_icon = menu_icon_texture(kFabIconPath);
        }
    }

    if (!menu_expanded()) {
        draw_menu_fab(ui, g_fab_icon);
        return;
    }

    menu_apply_resize_constraints();
    try_apply_initial_menu_layout();

    static bool s_shell_open = true;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(dp_to_px(10.f), dp_to_px(10.f)));
    const bool begun = ImGui::Begin(resolve_window_title(ui), &s_shell_open, flags);
    ImGui::PopStyleVar();

    if (!begun) {
        if (!s_shell_open) {
            set_menu_expanded(false);
            s_shell_open = true;
        }
        return;
    }

    if (!s_shell_open) {
        ImGui::End();
        set_menu_expanded(false);
        s_shell_open = true;
        return;
    }

    if (ui.tab_count <= 0) {
        ImGui::End();
        return;
    }

    static int s_selected = 0;
    if (s_selected < 0 || s_selected >= ui.tab_count) s_selected = 0;

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float gap = dp_to_px(kPanelGapDp);
    const float side_w = sidebar_width(avail.x);

    const ImVec2 side_size(side_w, avail.y);
    const ImVec2 content_size(std::max(32.f, avail.x - side_w - gap), avail.y);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, 0.f));
    draw_sidebar(ui, s_selected, side_size);
    ImGui::SameLine(0.f, gap);
    draw_content_panel(ui, s_selected, content_size);
    ImGui::PopStyleVar();

    ImGui::End();
}

} // namespace modui
