#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "OverlaySnapshot.h"
#include "../FishingCatalog.h"
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>
#include <imgui.h>
#include <cstring>
#include <string>

namespace playtogether {

namespace {

static bool UiCheckbox(const char *label, bool *v) {
    bool changed = ImGui::Checkbox(label, v);
    if (changed) SaveConfig();
    return changed;
}

static void DrawRiskHint(const char *text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

static bool MatchSearch(const char *label, const char *search) {
    if (!search || !search[0]) return true;
    if (!label) return false;
    if (strstr(label, search)) return true;
    return false;
}

static const char *FindBaitLabel(const FishingCatalog::Snapshot &cat, int baitItemId) {
    for (int i = 0; i < cat.baitCount; i++) {
        if ((int) cat.baits[i].itemId == baitItemId) return cat.baits[i].label;
    }
    return nullptr;
}

static const char *FindZoneLabel(const FishingCatalog::Snapshot &cat, unsigned int zoneId) {
    for (int i = 0; i < cat.zoneCount; i++) {
        if (cat.zones[i].zoneId == zoneId) return cat.zones[i].label;
    }
    return nullptr;
}

static bool IsLevelKept(unsigned int levelId) {
    for (unsigned int id : gPLConfig.fishing.keepLevelIds) {
        if (id == levelId) return true;
    }
    return false;
}

static void SetLevelKept(unsigned int levelId, bool kept) {
    for (size_t i = 0; i < gPLConfig.fishing.keepLevelIds.size(); i++) {
        if (gPLConfig.fishing.keepLevelIds[i] == levelId) {
            if (!kept) gPLConfig.fishing.keepLevelIds.erase(gPLConfig.fishing.keepLevelIds.begin() + (long) i);
            return;
        }
    }
    if (kept) gPLConfig.fishing.keepLevelIds.push_back(levelId);
}

static bool DrawLevelPicker() {
    static char s_levelSearch[64] = {};
    bool changed = false;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    int keptCount = 0;
    for (unsigned int id : gPLConfig.fishing.keepLevelIds) {
        if (id > 0) keptCount++;
    }
    char summary[64];
    snprintf(summary, sizeof(summary), OBF("Đã chọn %d level"), keptCount);
    ImGui::TextUnformatted(summary);
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint(OBF("##level_search"), OBF("Tên/ID…"), s_levelSearch, sizeof(s_levelSearch));
    if (!cat.ready) {
        ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        return false;
    }
    if (cat.levelCount <= 0) {
        ImGui::TextUnformatted(OBF("Chưa có level (vào game?)"));
        return false;
    }
    FishingCatalog::NotifyPickerOpen();
    ImGui::BeginChild(OBF("level_list##child"), ImVec2(0, 220), true);
    for (int i = 0; i < cat.levelCount; i++) {
        const auto &e = cat.levels[i];
        if (!MatchSearch(e.label, s_levelSearch)) {
            char idBuf[16];
            snprintf(idBuf, sizeof(idBuf), "%u", e.levelId);
            if (!MatchSearch(idBuf, s_levelSearch)) continue;
        }
        ImGui::PushID(i);
        bool kept = IsLevelKept(e.levelId);
        if (ImGui::Checkbox(e.label, &kept)) {
            SetLevelKept(e.levelId, kept);
            changed = true;
        }
        if (e.learnedFishCount > 0) {
            ImGui::Indent();
            ImGui::TextUnformatted(OBF("Cá đã học:"));
            for (int j = 0; j < e.learnedFishCount; j++) ImGui::BulletText("%s", e.learnedFish[j]);
            ImGui::Unindent();
        } else if (gPLConfig.fishing.filterByLevel) {
            ImGui::Indent();
            ImGui::TextUnformatted(OBF("(chưa học cá — câu để ghi nhận)"));
            ImGui::Unindent();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    return changed;
}

static bool DrawBaitPicker(const char *id, int *baitItemId) {
    if (!baitItemId) return false;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    char preview[128];
    if (*baitItemId > 0) {
        const char *lbl = cat.ready ? FindBaitLabel(cat, *baitItemId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Mồi ID %d"), *baitItemId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Mồi##bait_combo"), preview)) {
        FishingCatalog::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.baitCount <= 0) ImGui::TextUnformatted(OBF("Không có mồi trong túi"));
        for (int i = 0; i < cat.baitCount; i++) {
            const auto &e = cat.baits[i];
            ImGui::PushID(i);
            bool selected = ((int) e.itemId == *baitItemId);
            if (ImGui::Selectable(e.label, selected)) {
                *baitItemId = (int) e.itemId;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::InputInt(OBF("Hoặc ID tay##bait_manual"), baitItemId, 0, 0);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::PopID();
    return changed;
}

static bool DrawZonePicker(const char *id, unsigned int *zoneId) {
    if (!zoneId) return false;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    char preview[128];
    if (*zoneId > 0) {
        const char *lbl = cat.ready ? FindZoneLabel(cat, *zoneId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Vùng %u"), *zoneId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(tự / chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Vùng câu##zone_combo"), preview)) {
        FishingCatalog::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.zoneCount <= 0) ImGui::TextUnformatted(OBF("Chưa có vùng (vào game?)"));
        if (ImGui::Selectable(OBF("(Không chọn / 0)"), *zoneId == 0)) {
            *zoneId = 0;
            changed = true;
        }
        for (int i = 0; i < cat.zoneCount; i++) {
            const auto &e = cat.zones[i];
            ImGui::PushID(i);
            bool selected = (e.zoneId == *zoneId);
            if (ImGui::Selectable(e.label, selected)) {
                *zoneId = e.zoneId;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    int manual = (int) *zoneId;
    ImGui::InputInt(OBF("Hoặc ID tay##zone_manual"), &manual, 0, 0);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        *zoneId = (unsigned int) (manual < 0 ? 0 : manual);
        changed = true;
    }
    ImGui::PopID();
    return changed;
}

static bool DrawZoneBaitPrefEditor() {
    static unsigned int s_pickZone = 0;
    static int s_pickBait = 0;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    ImGui::TextUnformatted(OBF("Ưu tiên mồi theo vùng:"));
    if (DrawZonePicker(OBF("pref_zone"), &s_pickZone)) changed = true;
    if (DrawBaitPicker(OBF("pref_bait"), &s_pickBait)) changed = true;
    if (ImGui::Button(OBF("Thêm/cập nhật ưu tiên##zone_bait_add"), ImVec2(-1, 0))) {
        if (s_pickZone > 0 && s_pickBait > 0) {
            bool found = false;
            for (auto &p : gPLConfig.fishing.baitZonePrefs) {
                if (p.first == s_pickZone) {
                    p.second = (unsigned int) s_pickBait;
                    found = true;
                    break;
                }
            }
            if (!found) gPLConfig.fishing.baitZonePrefs.emplace_back(s_pickZone, (unsigned int) s_pickBait);
            changed = true;
        }
    }
    for (size_t i = 0; i < gPLConfig.fishing.baitZonePrefs.size(); i++) {
        const auto &p = gPLConfig.fishing.baitZonePrefs[i];
        const char *zLbl = FindZoneLabel(cat, p.first);
        const char *bLbl = FindBaitLabel(cat, (int) p.second);
        ImGui::PushID((int) i);
        ImGui::Text(OBF("• %s → %s"), zLbl ? zLbl : OBF("?"), bLbl ? bLbl : OBF("?"));
        ImGui::SameLine();
        if (ImGui::SmallButton(OBF("Xóa##rm_pref"))) {
            gPLConfig.fishing.baitZonePrefs.erase(gPLConfig.fishing.baitZonePrefs.begin() + (long) i);
            changed = true;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    return changed;
}

static void DrawTabFish() {
    ImGui::PushID(OBF("tab_fish"));
    UiCheckbox(OBF("Bật câu cá tự động"), &gPLConfig.fishing.enabled);
    UiCheckbox(OBF("Dừng khi hết lượt câu"), &gPLConfig.fishing.stopWhenCountOver);
    ImGui::PopID();
}

static void DrawTabFilter() {
    ImGui::PushID(OBF("tab_filter"));
    ImGui::TextUnformatted(OBF("Lọc theo bóng"));
    UiCheckbox(OBF("Bật lọc bóng"), &gPLConfig.fishing.filterByShadow);
    if (gPLConfig.fishing.filterByShadow) {
        static const char *kShadowLabels[7] = {OBF("S##sh1"), OBF("M##sh2"), OBF("L##sh3"), OBF("XL##sh4"), OBF("XXL##sh5"), OBF("XXXL##sh6"), OBF("4XL##sh7")};
        ImGui::TextUnformatted(OBF("Giữ bóng:"));
        for (int i = 0; i < 7; i++) {
            ImGui::PushID(i);
            if (ImGui::Checkbox(kShadowLabels[i], &gPLConfig.fishing.keepShadow[i])) SaveConfig();
            if (i < 6) ImGui::SameLine();
            ImGui::PopID();
        }
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Lọc theo level"));
    UiCheckbox(OBF("Bật lọc level"), &gPLConfig.fishing.filterByLevel);
    if (DrawLevelPicker()) SaveConfig();
    if (ImGui::CollapsingHeader(OBF("Khoảng level (nâng cao)##fish_lv_range"))) {
        ImGui::InputInt(OBF("Level tối thiểu (0=tắt)##fish_lv_min"), &gPLConfig.fishing.levelMin, 0, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::InputInt(OBF("Level tối đa (0=tắt)##fish_lv_max"), &gPLConfig.fishing.levelMax, 0, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::TextUnformatted(OBF("Chỉ dùng khi keepLevelIds rỗng"));
    }
    ImGui::PopID();
}

static void DrawTabBag() {
    ImGui::PushID(OBF("tab_bag"));
    UiCheckbox(OBF("Đóng hộp thưởng"), &gPLConfig.fishing.autoCloseReward);
    if (ImGui::Checkbox(OBF("Tự bán cá rác (rủi ro)"), &gPLConfig.fishing.autoSellTrash)) SaveConfig();
    if (gPLConfig.fishing.autoSellTrash) {
        DrawRiskHint(OBF("Cảnh báo: gọi bán qua dialog — có thể lệch server"));
        ImGui::SliderInt(OBF("Bán tối đa grade##fish_sell_grade"), &gPLConfig.fishing.maxSellGrade, 1, 3);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    if (ImGui::Checkbox(OBF("Bán/giữ theo túi (rủi ro)"), &gPLConfig.fishing.smartKeepSell)) SaveConfig();
    if (gPLConfig.fishing.smartKeepSell) {
        DrawRiskHint(OBF("Cảnh báo: giá trị + codex + áp lực túi"));
        ImGui::SliderInt(OBF("Giữ grade ≥##fish_keep_grade"), &gPLConfig.fishing.smartKeepMinGrade, 3, 5);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::SliderInt(OBF("Giữ nếu sở hữu <##fish_keep_cnt"), &gPLConfig.fishing.smartKeepMaxOwned, 1, 20);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::SliderInt(OBF("Bán nếu giá <##fish_min_sell"), &gPLConfig.fishing.minSellValue, 0, 5000);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        UiCheckbox(OBF("Giữ cá thiếu codex"), &gPLConfig.fishing.keepCodexFish);
    }
    ImGui::PopID();
}

static void DrawTabAdvanced() {
    ImGui::PushID(OBF("tab_adv"));
    ImGui::TextUnformatted(OBF("Mồi & zone"));
    if (ImGui::Checkbox(OBF("Tự gắn mồi UID (rủi ro)"), &gPLConfig.fishing.autoEquipBait)) SaveConfig();
    if (gPLConfig.fishing.autoEquipBait) {
        DrawRiskHint(OBF("Cảnh báo: set_FishingBaitUID trước cast"));
        if (DrawBaitPicker(OBF("main_bait"), &gPLConfig.fishing.baitItemId)) SaveConfig();
        UiCheckbox(OBF("Mồi theo zone (config JSON)"), &gPLConfig.fishing.smartBaitByZone);
        UiCheckbox(OBF("Mồi auto EffectId/ActionId"), &gPLConfig.fishing.smartBaitAutoEffect);
        if (gPLConfig.fishing.smartBaitByZone) {
            if (DrawZoneBaitPrefEditor()) SaveConfig();
        }
    }
    ImGui::PopID();
}

static void DrawTabSettings() {
    ImGui::PushID(OBF("tab_settings"));
    UiCheckbox(OBF("Bảng thông tin"), &gPLConfig.general.isInfo);
    ImGui::Separator();
    if (ImGui::Button(OBF("Lưu##settings_save"), ImVec2(-1, 0))) SaveConfig();
    if (ImGui::Button(OBF("Tải##settings_load"), ImVec2(-1, 0))) LoadConfig();
    ImGui::Separator();
    OverlaySnapshot::View snap{};
    OverlaySnapshot::Read(snap);
    if (snap.ready) {
        ImGui::Text(OBF("Map: %d"), snap.mapId);
        ImGui::Text(OBF("Tọa độ: %.1f, %.1f, %.1f"), snap.position.x, snap.position.y, snap.position.z);
    } else {
        ImGui::TextUnformatted(OBF("Map: —"));
        ImGui::TextUnformatted(OBF("Tọa độ: —"));
    }
    ImGui::PopID();
}

static void DrawSubTabScroll(const char *id, void (*draw)()) {
    ImGui::PushID(id);
    ImGui::BeginChild(OBF("##sub_scroll"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    draw();
    ImGui::EndChild();
    ImGui::PopID();
}

static void DrawFishingPage() {
    ImGui::PushID(OBF("page_fishing"));
    if (ImGui::BeginTabBar(OBF("##pl_fishing_tabs"), ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem(OBF("Câu"))) {
            DrawSubTabScroll(OBF("sub_fish"), DrawTabFish);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lọc"))) {
            DrawSubTabScroll(OBF("sub_filter"), DrawTabFilter);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Cá & Túi"))) {
            DrawSubTabScroll(OBF("sub_bag"), DrawTabBag);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Nâng cao"))) {
            DrawSubTabScroll(OBF("sub_adv"), DrawTabAdvanced);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Cài đặt"))) {
            DrawSubTabScroll(OBF("sub_settings"), DrawTabSettings);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopID();
}

}

void SetupMenuUi() {
    modui::AppUi ui{};
    ui.menu_size = ImVec2(720.f, 520.f);
    ui.fab_icon_path = OBF("/data/user/0/") + Tools::GetPackageName() + OBF("/files/fab.png");
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("fishing"), OBF("Câu Cá"), DrawFishingPage);
    modui::SetAppUi(ui);
}

}
