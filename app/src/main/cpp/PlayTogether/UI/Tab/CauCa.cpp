#include "CauCa.h"
#include "../../Config/Config.h"
#include "../../Hook/AutoFishing/PickerSnapshot.h"
#include <Includes/obfuscate.h>
#include <imgui.h>
#include <cstring>
#include <string>

namespace playtogether {

namespace {

bool UiCheckbox(const char *label, bool *v) {
    bool changed = ImGui::Checkbox(label, v);
    if (changed) SaveConfig();
    return changed;
}

const char *PickerBaitLabel(const AutoFishing::PickerSnapshot &cat, int itemId) {
    for (int i = 0; i < cat.baitCount; i++) {
        if ((int) cat.baits[i].itemId == itemId) return cat.baits[i].label;
    }
    return nullptr;
}

const char *PickerZoneLabel(const AutoFishing::PickerSnapshot &cat, unsigned int zoneId) {
    for (int i = 0; i < cat.zoneCount; i++) {
        if (cat.zones[i].zoneId == zoneId) return cat.zones[i].label;
    }
    return nullptr;
}

bool DrawBaitPicker(const char *id, int *baitItemId) {
    if (!baitItemId) return false;
    AutoFishing::PickerSnapshot cat{};
    AutoFishing::ReadPicker(cat);
    bool changed = false;
    char preview[128];
    if (*baitItemId > 0) {
        const char *lbl = cat.ready ? PickerBaitLabel(cat, *baitItemId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
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
    ImGui::PopID();
    return changed;
}

bool DrawZonePicker(const char *id, unsigned int *zoneId) {
    if (!zoneId) return false;
    AutoFishing::PickerSnapshot cat{};
    AutoFishing::ReadPicker(cat);
    bool changed = false;
    char preview[128];
    if (*zoneId > 0) {
        const char *lbl = cat.ready ? PickerZoneLabel(cat, *zoneId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Vùng câu##zone_combo"), preview)) {
        AutoFishing::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.zoneCount <= 0) ImGui::TextUnformatted(OBF("Chưa có vùng"));
        if (ImGui::Selectable(OBF("Không chọn"), *zoneId == 0)) {
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
    ImGui::PopID();
    return changed;
}

void DrawCraftBaitPanel() {
    UiCheckbox(OBF("Tự chế mồi"), &gPLConfig.fishing.autoCraftBait);

    int target = gPLConfig.fishing.craftBaitTargetCount;
    if (target < 1) target = 1;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputInt(OBF("##craft_qty"), &target, 1, 5)) {
        if (target < 1) target = 1;
        if (target > 999) target = 999;
        gPLConfig.fishing.craftBaitTargetCount = target;
        SaveConfig();
    }

    AutoFishing::PickerSnapshot cat{};
    AutoFishing::ReadPicker(cat);
    ImGui::TextUnformatted(OBF("Công thức mồi:"));

    ImGui::BeginChild(OBF("craft_recipe_list##scroll"), ImVec2(-1, 500.f), ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle);
    if (!cat.ready || cat.baitRecipeCount <= 0) {
        ImGui::TextUnformatted(cat.ready ? OBF("Không có công thức mồi") : OBF("Đang tải danh sách…"));
    } else {
        bool changed = false;
        const unsigned sel = gPLConfig.fishing.craftBaitItemId;
        bool selValid = sel == 0;
        for (int i = 0; i < cat.baitRecipeCount; i++) {
            const auto &e = cat.baitRecipes[i];
            if (e.itemId == sel) selValid = true;
            ImGui::PushID(i);
            if (ImGui::Selectable(e.label, sel == e.itemId)) {
                gPLConfig.fishing.craftBaitItemId = e.itemId;
                changed = true;
            }
            ImGui::PopID();
        }
        if (!selValid) {
            gPLConfig.fishing.craftBaitItemId = 0;
            changed = true;
        }
        if (changed) SaveConfig();
    }
    ImGui::EndChild();
}

} // namespace

void DrawCauCaPage() {
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
            UiCheckbox(OBF("Bảo Quản"), &gPLConfig.fishing.autoCloseReward);
            ImGui::TableNextColumn();
            ImGui::EndTable();
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
            ImGui::TextUnformatted(OBF("Lọc Theo ID"));
            UiCheckbox(OBF("Bật lọc ID"), &gPLConfig.fishing.filterByLevel);
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

    if (ImGui::BeginTabItem(OBF("Bán Cá"))) {
        ImGui::BeginChild(OBF("sub_sell##scroll"), ImVec2(0, 0));
        if (ImGui::BeginTable(OBF("##sell_cols"), 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(OBF("Bán theo bóng"));
            UiCheckbox(OBF("Bật bán theo bóng"), &gPLConfig.fishing.sellByShadow);
            if (gPLConfig.fishing.sellByShadow) {
                ImGui::TextUnformatted(OBF("Bán bóng:"));
                for (int i = 0; i < 7; i++) {
                    char label[16];
                    snprintf(label, sizeof(label), OBF("Bóng %d"), i + 1);
                    ImGui::PushID(i);
                    if (ImGui::Checkbox(label, &gPLConfig.fishing.sellShadow[i])) SaveConfig();
                    ImGui::PopID();
                }
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(OBF("Bán theo độ hiếm"));
            UiCheckbox(OBF("Bật bán theo màu nền"), &gPLConfig.fishing.sellByGrade);
            if (gPLConfig.fishing.sellByGrade) {
                static const char *kSellGradeLabels[5] = {
                        OBF("Trắng - Thường"),
                        OBF("Xanh lá - Cao cấp"),
                        OBF("Xanh dương - Hiếm"),
                        OBF("Tím - Huyền thoại"),
                        OBF("Cực quang - Truyền thuyết"),
                };
                ImGui::TextUnformatted(OBF("Bán khi khớp:"));
                for (int i = 0; i < 5; i++) {
                    ImGui::PushID(i);
                    if (ImGui::Checkbox(kSellGradeLabels[i], &gPLConfig.fishing.sellGrade[i])) SaveConfig();
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(OBF("Nâng cao"))) {
        ImGui::BeginChild(OBF("sub_adv##scroll"), ImVec2(0, 0));

        if (ImGui::CollapsingHeader(OBF("Gắn mồi##adv_equip"))) {
            if (ImGui::Checkbox(OBF("Tự gắn mồi"), &gPLConfig.fishing.autoEquipBait)) SaveConfig();
            if (gPLConfig.fishing.autoEquipBait) {
                if (DrawBaitPicker(OBF("main_bait"), &gPLConfig.fishing.baitItemId)) SaveConfig();
            }
        }

        ImGui::Spacing();
        if (ImGui::CollapsingHeader(OBF("Fake vùng câu##adv_zone"))) {
            if (UiCheckbox(OBF("Bật fake vùng"), &gPLConfig.fishing.fakeZoneEnabled)) {}
            if (gPLConfig.fishing.fakeZoneEnabled) {
                if (DrawZonePicker(OBF("fake_zone"), &gPLConfig.fishing.fakeZoneId)) SaveConfig();
            }
        }

        ImGui::Spacing();
        if (ImGui::CollapsingHeader(OBF("Chế mồi##adv_craft"))) {
            AutoFishing::NotifyCraftPanelVisible();
            DrawCraftBaitPanel();
        }

        ImGui::EndChild();
        ImGui::EndTabItem();
    } else {
        AutoFishing::NotifyPickerClosed();
    }

    ImGui::EndTabBar();
    ImGui::PopID();
}

}
