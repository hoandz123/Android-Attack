#include "MenuUi.h"
#include "Config/Config.h"
#include "../Hook/AutoFishing/PickerSnapshot.h"
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

static const char *PickerBaitLabel(const AutoFishing::PickerSnapshot &cat, int itemId) {
    for (int i = 0; i < cat.baitCount; i++) {
        if ((int) cat.baits[i].itemId == itemId) return cat.baits[i].label;
    }
    return nullptr;
}

static const char *PickerZoneLabel(const AutoFishing::PickerSnapshot &cat, unsigned int zoneId) {
    for (int i = 0; i < cat.zoneCount; i++) {
        if (cat.zones[i].zoneId == zoneId) return cat.zones[i].label;
    }
    return nullptr;
}

static bool DrawBaitPicker(const char *id, int *baitItemId) {
    if (!baitItemId) return false;
    AutoFishing::PickerSnapshot cat{};
    AutoFishing::ReadPicker(cat);
    bool changed = false;
    char preview[128];
    if (*baitItemId > 0) {
        const char *lbl = cat.ready ? PickerBaitLabel(cat, *baitItemId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Mồi ID %d"), *baitItemId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Mồi##bait_combo"), preview)) {
        AutoFishing::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.baitCount <= 0) ImGui::TextUnformatted(OBF("Không có mồi trong túi"));
        for (int i = 0; i < cat.baitCount; i++) {
            const auto &e = cat.baits[i];
            ImGui::PushID(i);
            if (ImGui::Selectable(e.label, (int) e.itemId == *baitItemId)) {
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
    AutoFishing::PickerSnapshot cat{};
    AutoFishing::ReadPicker(cat);
    bool changed = false;
    char preview[128];
    if (*zoneId > 0) {
        const char *lbl = cat.ready ? PickerZoneLabel(cat, *zoneId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Vùng %u"), *zoneId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(tự / chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Vùng câu##zone_combo"), preview)) {
        AutoFishing::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.zoneCount <= 0) ImGui::TextUnformatted(OBF("Chưa có vùng (vào game?)"));
        if (ImGui::Selectable(OBF("(Không chọn / 0)"), *zoneId == 0)) {
            *zoneId = 0;
            changed = true;
        }
        for (int i = 0; i < cat.zoneCount; i++) {
            const auto &e = cat.zones[i];
            ImGui::PushID(i);
            if (ImGui::Selectable(e.label, e.zoneId == *zoneId)) {
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

static void DrawFishingPage() {
    ImGui::PushID(OBF("page_fishing"));
    if (!ImGui::BeginTabBar(OBF("##pl_fishing_tabs"), ImGuiTabBarFlags_None)) {
        ImGui::PopID();
        return;
    }

    if (ImGui::BeginTabItem(OBF("Câu"))) {
        ImGui::BeginChild(OBF("sub_fish##scroll"), ImVec2(0, 0));
        if (ImGui::BeginTable(OBF("##fish_grid"), 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            UiCheckbox(OBF("Auto câu"), &gPLConfig.fishing.enabled);
            ImGui::TableNextColumn();
            UiCheckbox(OBF("Hết lượt"), &gPLConfig.fishing.stopWhenCountOver);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            UiCheckbox(OBF("Đóng thưởng"), &gPLConfig.fishing.autoCloseReward);
            ImGui::TableNextColumn();
            if (ImGui::Checkbox(OBF("Bán rác"), &gPLConfig.fishing.autoSellTrash)) SaveConfig();
            ImGui::EndTable();
        }
        if (gPLConfig.fishing.autoSellTrash) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
            ImGui::TextUnformatted(OBF("Dialog bán — có thể lệch server"));
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderInt(OBF("Grade bán##fish_sell_grade"), &gPLConfig.fishing.maxSellGrade, 1, 3);
            if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(OBF("Lọc"))) {
        ImGui::BeginChild(OBF("sub_filter##scroll"), ImVec2(0, 0));
        if (ImGui::BeginTable(OBF("##filter_cols"), 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(OBF("Lọc theo bóng"));
            UiCheckbox(OBF("Bật lọc bóng"), &gPLConfig.fishing.filterByShadow);
            if (gPLConfig.fishing.filterByShadow) {
                ImGui::TextUnformatted(OBF("Giữ bóng:"));
                for (int i = 0; i < 7; i++) {
                    char label[16];
                    snprintf(label, sizeof(label), OBF("Bóng %d"), i + 1);
                    ImGui::PushID(i);
                    if (ImGui::Checkbox(label, &gPLConfig.fishing.keepShadow[i])) SaveConfig();
                    ImGui::PopID();
                }
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(OBF("Lọc theo level"));
            UiCheckbox(OBF("Bật lọc level"), &gPLConfig.fishing.filterByLevel);
            if (gPLConfig.fishing.filterByLevel) {
                static char keepLevelsBuf[128] = {};
                static std::string keepLevelsSynced;
                if (keepLevelsSynced != gPLConfig.fishing.keepLevels) {
                    std::strncpy(keepLevelsBuf, gPLConfig.fishing.keepLevels.c_str(), sizeof(keepLevelsBuf) - 1);
                    keepLevelsBuf[sizeof(keepLevelsBuf) - 1] = '\0';
                    keepLevelsSynced = gPLConfig.fishing.keepLevels;
                }
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint(OBF("##keep_levels"), OBF("VD: 1,2,3,4"), keepLevelsBuf, sizeof(keepLevelsBuf));
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    gPLConfig.fishing.keepLevels = keepLevelsBuf;
                    keepLevelsSynced = gPLConfig.fishing.keepLevels;
                    SaveConfig();
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(OBF("Nâng cao"))) {
        ImGui::BeginChild(OBF("sub_adv##scroll"), ImVec2(0, 0));
        ImGui::TextUnformatted(OBF("Mồi & zone"));
        if (ImGui::Checkbox(OBF("Tự gắn mồi UID (rủi ro)"), &gPLConfig.fishing.autoEquipBait)) SaveConfig();
        if (gPLConfig.fishing.autoEquipBait) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
            ImGui::TextUnformatted(OBF("Cảnh báo: set_FishingBaitUID trước cast"));
            ImGui::PopStyleColor();
            if (DrawBaitPicker(OBF("main_bait"), &gPLConfig.fishing.baitItemId)) SaveConfig();
            UiCheckbox(OBF("Mồi theo zone (config JSON)"), &gPLConfig.fishing.smartBaitByZone);
            UiCheckbox(OBF("Mồi auto EffectId/ActionId"), &gPLConfig.fishing.smartBaitAutoEffect);
            if (gPLConfig.fishing.smartBaitByZone) {
                static unsigned int pickZone = 0;
                static int pickBait = 0;
                AutoFishing::PickerSnapshot cat{};
                AutoFishing::ReadPicker(cat);
                bool changed = false;
                ImGui::TextUnformatted(OBF("Ưu tiên mồi theo vùng:"));
                if (DrawZonePicker(OBF("pref_zone"), &pickZone)) changed = true;
                if (DrawBaitPicker(OBF("pref_bait"), &pickBait)) changed = true;
                if (ImGui::Button(OBF("Thêm/cập nhật ưu tiên##zone_bait_add"), ImVec2(-1, 0)) && pickZone > 0 && pickBait > 0) {
                    bool found = false;
                    for (auto &p : gPLConfig.fishing.baitZonePrefs) {
                        if (p.first == pickZone) {
                            p.second = (unsigned int) pickBait;
                            found = true;
                            break;
                        }
                    }
                    if (!found) gPLConfig.fishing.baitZonePrefs.emplace_back(pickZone, (unsigned int) pickBait);
                    changed = true;
                }
                for (size_t i = 0; i < gPLConfig.fishing.baitZonePrefs.size(); i++) {
                    const auto &p = gPLConfig.fishing.baitZonePrefs[i];
                    const char *zLbl = PickerZoneLabel(cat, p.first);
                    const char *bLbl = PickerBaitLabel(cat, (int) p.second);
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
                if (changed) SaveConfig();
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
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
