#include "ModUi.hpp"
#include "Internal.hpp"
#include "Layout.hpp"
#include "Icon.hpp"
#include <Includes/obfuscate.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace modui {

namespace {

constexpr float kSidebarMaxWidthFrac = 0.50f;
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
constexpr float kDefaultMenuWidthDp = 624.f;
constexpr float kDefaultMenuHeightDp = 442.f;
constexpr int kMaxTabAnim = 16;
constexpr float kAnimSpeed = 16.f;

const char *DefaultWindowTitle() { return OBF("Mod Menu##modui_shell"); }
const char *FabWindowId() { return OBF("##modui_fab"); }

struct SidebarTabAnimState {
    float blend[kMaxTabAnim]{};
};

SidebarTabAnimState g_tab_anim;

void ClampFabPos(ImVec2 &pos, float fab_sz) {
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 safe = ImGui::GetStyle().DisplaySafeAreaPadding;
    const float pad = DpToPx(kFabEdgePadDp);
    const float x0 = vp->WorkPos.x + safe.x + pad;
    const float y0 = vp->WorkPos.y + safe.y + pad;
    const float x1 = vp->WorkPos.x + vp->WorkSize.x - fab_sz - safe.x - pad;
    const float y1 = vp->WorkPos.y + vp->WorkSize.y - fab_sz - safe.y - pad;
    pos.x = std::clamp(pos.x, x0, std::max(x0, x1));
    pos.y = std::clamp(pos.y, y0, std::max(y0, y1));
}

float SmoothToward(float current, float target, float dt) {
    const float k = 1.f - std::exp(-kAnimSpeed * dt);
    return current + (target - current) * k;
}

ImU32 ColorToU32(const ImVec4 &c) {
    return ImGui::ColorConvertFloat4ToU32(c);
}

float SidebarWidth(const AppUi &ui, float content_w) {
    const float min_w = DpToPx(kSidebarMinDp);
    const float max_w = content_w * kSidebarMaxWidthFrac;
    const float pad_x = DpToPx(kTabPadXDp);
    const float ribbon_w = DpToPx(kRibbonWidthDp);
    const float gap = DpToPx(kPanelGapDp);
    const float border = ImGui::GetStyle().ChildBorderSize;
    float max_text = 0.f;
    for (int i = 0; i < ui.tab_count; ++i) {
        const char *label = ui.tabs[i].label ? ui.tabs[i].label : "";
        max_text = std::max(max_text, ImGui::CalcTextSize(label).x);
    }
    const float inner = max_text + pad_x + ribbon_w;
    const float w = inner + gap * 2.f + border * 2.f;
    return std::clamp(w, min_w, std::max(min_w, max_w));
}

ImVec2 ResolveMenuWindowSize(const AppUi &ui) {
    const MenuLayoutConfig &cfg = GetMenuLayout();
    float w_dp = kDefaultMenuWidthDp;
    float h_dp = kDefaultMenuHeightDp;
    if (ui.menu_size.x > 0.f && ui.menu_size.y > 0.f) {
        w_dp = ui.menu_size.x;
        h_dp = ui.menu_size.y;
    }
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 work = vp->WorkSize;
    float w = DpToPx(w_dp);
    float h = DpToPx(h_dp);
    const float min_w = DpToPx(cfg.min_width_dp);
    const float min_h = DpToPx(cfg.min_height_dp);
    const float max_w = work.x * cfg.max_width_screen;
    const float max_h = work.y * cfg.max_height_screen;
    w = std::clamp(w, min_w, max_w);
    h = std::clamp(h, min_h, max_h);
    return ImVec2(w, h);
}

bool DrawSidebarTabRibbon(int index, const char *label, bool selected, float row_h,
                             float width) {
    if (index < 0 || index >= kMaxTabAnim) return false;

    const float dt = ImGui::GetIO().DeltaTime;
    g_tab_anim.blend[index] = SmoothToward(g_tab_anim.blend[index], selected ? 1.f : 0.f, dt);

    const float round = DpToPx(kTabRoundDp);
    const float pad_x = DpToPx(kTabPadXDp);
    const float ribbon_w = DpToPx(kRibbonWidthDp);
    const float ribbon_inset = DpToPx(kRibbonInsetVDp);
    const ImVec2 size(width, row_h);

    ImGui::PushID(index);
    const bool pressed = ImGui::InvisibleButton(OBF("##tab"), size);
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
        const ImU32 right = ColorToU32(ImVec4(0.05f, 0.48f, 0.86f, 0.15f * sel));
        dl->AddRectFilledMultiColor(p0, p1, left, right, right, left);

        const float aw = ribbon_w * sel;
        const ImVec2 r0(p1.x - aw, p0.y + ribbon_inset);
        const ImVec2 r1(p1.x, p1.y - ribbon_inset);
        dl->AddRectFilled(r0, r1, ColorToU32(ImVec4(0.12f, 0.58f, 0.95f, sel)), 3.f,
                          ImDrawFlags_RoundCornersRight);
    }

    const char *text = label ? label : "";
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const float text_x = p0.x + pad_x;
    const float text_y = p0.y + (p1.y - p0.y - ts.y) * 0.5f;
    const ImVec4 text_idle(0.72f, 0.76f, 0.84f, 1.f);
    const ImVec4 text_sel(1.f, 1.f, 1.f, 1.f);
    const ImVec4 text_c(text_idle.x + (text_sel.x - text_idle.x) * sel,
                        text_idle.y + (text_sel.y - text_idle.y) * sel,
                        text_idle.z + (text_sel.z - text_idle.z) * sel,
                        text_idle.w + (text_sel.w - text_idle.w) * sel);
    dl->AddText(ImVec2(text_x, text_y), ColorToU32(text_c), text);

    return pressed;
}

void DrawSidebar(const AppUi &ui, int &selected, ImVec2 panel_size) {
    const float row_h = DpToPx(kTabRowDp);
    const float gap = DpToPx(kPanelGapDp);
    const float tab_gap = DpToPx(kTabGapDp);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.12f, 0.145f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(gap, gap));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, tab_gap));
    if (ImGui::BeginChild(OBF("modui_sidebar"), panel_size, ImGuiChildFlags_Borders)) {
        const float tab_w = ImGui::GetContentRegionAvail().x;

        for (int i = 0; i < ui.tab_count; ++i) {
            const MenuTab &tab = ui.tabs[i];
            const char *label = tab.label ? tab.label : "";
            const bool is_sel = (i == selected);
            if (DrawSidebarTabRibbon(i, label, is_sel, row_h, tab_w)) selected = i;
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void DrawContentPanel(const AppUi &ui, int selected, ImVec2 panel_size) {
    const float gap = DpToPx(kPanelGapDp);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.14f, 0.17f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(gap * 1.25f, gap * 1.25f));
    if (ImGui::BeginChild(OBF("modui_content"), panel_size, ImGuiChildFlags_Borders)) {
        if (selected >= 0 && selected < ui.tab_count) {
            const MenuTab &tab = ui.tabs[selected];
            if (tab.draw) tab.draw();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void DrawMenuFab(const AppUi &) {
    const float fab = DpToPx(kFabDp);
    const ImVec2 fab_sz(fab, fab);
    const bool has_tex = GetFabIcon() != (ImTextureID) 0;

    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 safe = ImGui::GetStyle().DisplaySafeAreaPadding;
    const float margin = DpToPx(kFabMarginDp);
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + margin + safe.x, vp->WorkPos.y + margin + safe.y), ImGuiCond_FirstUseEver);
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
    if (!ImGui::Begin(FabWindowId(), nullptr, flags)) {
        if (!has_tex) ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        return;
    }
    if (!has_tex) ImGui::PopStyleColor();

    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + fab_sz.x, p0.y + fab_sz.y);
    DrawFabIcon(ImGui::GetWindowDrawList(), p0, p1, fab * 0.5f);
    ImGui::Dummy(fab_sz);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(0)) {
        const ImVec2 d = ImGui::GetMouseDragDelta(0);
        const float th = ImGui::GetIO().MouseDragThreshold;
        if (d.x * d.x + d.y * d.y <= th * th) SetMenuExpanded(true);
    }

    ImVec2 pos = ImGui::GetWindowPos();
    ClampFabPos(pos, fab);
    const ImVec2 cur = ImGui::GetWindowPos();
    if (pos.x != cur.x || pos.y != cur.y) ImGui::SetWindowPos(pos);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace

void DrawMenuShell(const AppUi &ui) {
    if (!MenuVisible()) return;

    if (!MenuExpanded()) {
        DrawMenuFab(ui);
        return;
    }

    const ImVec2 menu_sz = ResolveMenuWindowSize(ui);
    ApplyResizeConstraints();
    ApplyInitialLayout(&menu_sz);

    static bool s_shell_open = true;

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(DpToPx(10.f), DpToPx(10.f)));
    const char *title = (ui.window_title && ui.window_title[0]) ? ui.window_title : DefaultWindowTitle();
    const bool begun = ImGui::Begin(title, &s_shell_open, flags);
    ImGui::PopStyleVar();

    if (!begun) {
        if (!s_shell_open) {
            SetMenuExpanded(false);
            s_shell_open = true;
        }
        return;
    }

    if (!s_shell_open) {
        ImGui::End();
        SetMenuExpanded(false);
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
    const float gap = DpToPx(kPanelGapDp);
    const float side_w = SidebarWidth(ui, avail.x);

    const ImVec2 side_size(side_w, avail.y);
    const ImVec2 content_size(std::max(32.f, avail.x - side_w - gap), avail.y);

    DrawSidebar(ui, s_selected, side_size);
    ImGui::SameLine(0.f, gap);
    DrawContentPanel(ui, s_selected, content_size);

    ImGui::End();
}

} // namespace modui
