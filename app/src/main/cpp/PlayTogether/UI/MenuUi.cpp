#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "FishLogger.h"
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>
#include <imgui.h>
#include <sstream>
#include <string>
#include <vector>

namespace playtogether {

namespace {

static bool UiCheckbox(const char *label, bool *v) {
    bool changed = ImGui::Checkbox(label, v);
    if (changed) SaveConfig();
    return changed;
}

static bool UiSliderInt(const char *label, int *v, int minV, int maxV) {
    bool changed = ImGui::SliderInt(label, v, minV, maxV);
    if (changed) SaveConfig();
    return changed;
}

static void ShowFishingHistory() {
    static std::vector<std::string> dates;
    static std::vector<FishingEntry> logs = PLConfig::FishingConfig::gFishLogger.LoadHistory(dates);
    static int dateSel = 0;
    if (ImGui::Button(OBF("Làm mới##fish_hist_refresh"))) {
        logs = PLConfig::FishingConfig::gFishLogger.LoadHistory(dates);
        dateSel = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button(OBF("Xóa LS##fish_hist_clear"))) {
        PLConfig::FishingConfig::gFishLogger.Delete();
        logs = PLConfig::FishingConfig::gFishLogger.LoadHistory(dates);
        dateSel = 0;
        SaveConfig();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(OBF("Chọn ngày"));
    ImGui::SameLine();
    if (ImGui::Combo(OBF("##ChonNgay"), &dateSel, [](void *vec, int idx, const char **out) -> bool {
        auto *v = (std::vector<std::string> *) vec;
        if (idx < 0 || idx >= (int) v->size()) return false;
        *out = (*v)[idx].c_str();
        return true;
    }, &dates, (int) dates.size()) && dateSel >= 0) SaveConfig();
    if (ImGui::BeginTabBar(OBF("##ResultTabs"))) {
        auto drawTable = [&](const char *id, const char *tabName, bool wantSuccess) {
            if (!ImGui::BeginTabItem(tabName)) return;
            if (ImGui::BeginTable(id, 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn(OBF("Tên"));
                ImGui::TableSetupColumn(OBF("ID"));
                ImGui::TableSetupColumn(OBF("Vùng"));
                ImGui::TableSetupColumn(OBF("Bóng"));
                ImGui::TableSetupColumn(OBF("Thời gian"));
                ImGui::TableHeadersRow();
                for (auto it = logs.rbegin(); it != logs.rend(); ++it) {
                    const FishingEntry &e = *it;
                    if (dateSel && e.ymd != dates[dateSel]) continue;
                    bool isSuccess = (e.result == OBF("success"));
                    if (wantSuccess != isSuccess) continue;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(e.name.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", e.level);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d", e.fishZone);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%d", e.grade);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::TextUnformatted(FishLogger::formatTime(e.ts).c_str());
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        };
        drawTable(OBF("HistSuccess"), OBF("Đã câu"), true);
        drawTable(OBF("HistFailure"), OBF("Đứt dây"), false);
        ImGui::EndTabBar();
    }
}

static void DrawTabCauCa() {
    auto &fishing = gPLConfig.fishing;
    if (ImGui::BeginTabBar(OBF("##FishingTabs"))) {
        if (ImGui::BeginTabItem(OBF("Auto"))) {
            UiCheckbox(OBF("Bật auto câu"), &fishing.isCauCa);
            ImGui::Separator();
            UiSliderInt(OBF("Delay (ms)"), &fishing.delayAutoMs, 100, 2000);
            ImGui::Text(OBF("Trạng thái: %s"), PLConfig::FishingConfig::curStateName.c_str());
            ImGui::Text(OBF("Đã câu: %d | Bỏ: %d | Hụt: %d"),
                PLConfig::FishingConfig::totalCaught,
                PLConfig::FishingConfig::totalSkipped,
                PLConfig::FishingConfig::totalFailed);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lọc cá"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("FishL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Lọc ID"), &fishing.isLocID);
            static char addIdBuf[128] = {};
            ImGui::InputText(OBF("##AddFishId"), addIdBuf, sizeof(addIdBuf));
            ImGui::SameLine();
            if (ImGui::Button(OBF("Thêm ID##fish_add_id"))) {
                std::istringstream ss(addIdBuf);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    if (token.empty()) continue;
                    try {
                        fishing.IDLocCa[std::stoi(token)] = true;
                    } catch (...) {}
                }
                addIdBuf[0] = '\0';
                SaveConfig();
            }
            for (auto it = fishing.IDLocCa.begin(); it != fishing.IDLocCa.end();) {
                std::string label = std::string(OBF("ID ")) + std::to_string(it->first) + OBF("##fishid") + std::to_string(it->first);
                UiCheckbox(label.c_str(), &it->second);
                ImGui::SameLine();
                std::string del = OBF("Xóa##") + std::to_string(it->first);
                if (ImGui::Button(del.c_str())) {
                    it = fishing.IDLocCa.erase(it);
                    SaveConfig();
                    continue;
                }
                ++it;
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("FishR"), ImVec2(0, avail.y), true);
            bool isOff = true;
            for (int i = 0; i < 7; i++) if (fishing.locBong[i]) isOff = false;
            if (UiCheckbox(OBF("Tắt lọc bóng"), &isOff) && isOff) for (int i = 0; i < 7; i++) fishing.locBong[i] = false;
            for (int i = 0; i < 7; i++) {
                std::string lbl = OBF("Bóng ") + std::to_string(i + 1) + OBF("##bong") + std::to_string(i);
                UiCheckbox(lbl.c_str(), &fishing.locBong[i]);
            }
            ImGui::Separator();
            const char *grades[] = {OBF("Tắt"), OBF("F"), OBF("D"), OBF("C"), OBF("B"), OBF("A"), OBF("S")};
            ImGui::TextUnformatted(OBF("Roll cần"));
            if (ImGui::BeginCombo(OBF("##gradeCombo"), grades[fishing.RollCapDo])) {
                for (int n = 0; n < 7; n++) {
                    bool selected = fishing.RollCapDo == n;
                    ImGui::PushID(n);
                    if (ImGui::Selectable(grades[n], selected)) {
                        fishing.RollCapDo = n;
                        SaveConfig();
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Bán cá"))) {
            if (UiCheckbox(OBF("Bảo quản"), &fishing.isBaoQuan) && fishing.isBaoQuan) fishing.isBanGoi = false;
            ImGui::Separator();
            if (UiCheckbox(OBF("Bán nhanh"), &fishing.isBanGoi) && fishing.isBanGoi) fishing.isBaoQuan = false;
            if (fishing.isBanGoi) {
                if (UiCheckbox(OBF("Giữ Tím+VVIP"), &fishing.isDuNenVip) && fishing.isDuNenVip) fishing.isDuB67 = false;
                ImGui::Separator();
                if (UiCheckbox(OBF("Giữ bóng 6–7"), &fishing.isDuB67) && fishing.isDuB67) fishing.isDuNenVip = false;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Nước phép"))) {
            auto &magicWater = fishing.magicWater;
            UiCheckbox(OBF("Tự dùng nước phép"), &magicWater.isEnable);
            ImGui::TextUnformatted(OBF("Nước phép (tối đa 5)"));
            int totalUses = magicWater.levelUses[0] + magicWater.levelUses[1] + magicWater.levelUses[2];
            ImGui::Text(OBF("Tổng: %d/5"), totalUses);
            const char *levels[] = {OBF("Cấp 1"), OBF("Cấp 2"), OBF("Cấp 3")};
            for (int i = 0; i < 3; i++) {
                ImGui::Separator();
                ImGui::TextUnformatted(levels[i]);
                ImGui::SameLine();
                std::string minusId = OBF("-##MW") + std::to_string(i);
                std::string plusId = OBF("+##MW") + std::to_string(i);
                if (ImGui::Button(minusId.c_str()) && magicWater.levelUses[i] > 0) {
                    magicWater.levelUses[i]--;
                    SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button(plusId.c_str()) && totalUses < 5) {
                    magicWater.levelUses[i]++;
                    SaveConfig();
                }
                ImGui::SameLine();
                ImGui::Text(OBF("SL: %d"), magicWater.levelUses[i]);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Fake zone [⚠]"))) {
            ImGui::TextColored(ImVec4(1.f, 0.3f, 0.2f, 1.f), "%s", OBF("Cảnh báo: fake zone có thể ban/crash. Tự chịu trách nhiệm."));
            if (UiCheckbox(OBF("Fake rồng (503)"), &fishing.isFakeVR) && fishing.isFakeVR) fishing.isFishZone = false;
            UiSliderInt(OBF("ID vùng"), &fishing.fishZone, 0, 9999);
            UiCheckbox(OBF("Bật fake zone"), &fishing.isFishZone);
            if (fishing.isFishZone && !fishing.isFakeVR && fishing.fishZone <= 0) {
                ImGui::TextColored(ImVec4(1.f, 0.6f, 0.1f, 1.f), "%s", OBF("Cần ID vùng > 0"));
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("ESP"))) {
            auto &esp = fishing.esp;
            UiCheckbox(OBF("Bật ESP câu cá"), &esp.isEnable);
            if (esp.isEnable) {
                UiCheckbox(OBF("Hiện vùng câu"), &esp.isShowZone);
                UiCheckbox(OBF("Bảng trạng thái"), &esp.isShowStatus);
            }
            ImGui::Separator();
            UiCheckbox(OBF("Bảng thông tin"), &gPLConfig.general.isInfo);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lịch sử"))) {
            ShowFishingHistory();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabMap() {
    if (ImGui::BeginTabBar(OBF("##MapTabs"))) {
        if (ImGui::BeginTabItem(OBF("Bản đồ"))) {
            ImGui::BeginChild(OBF("MapList"), ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0), true);
            for (const PLConfig::MapInfo &map : PLConfig::GetMapInfoList()) {
                std::string label = map.name;
                if (PLConfig::GetPlayerMapID() == map.id) label += OBF(" [đây]");
                label += OBF("##mapbtn") + std::to_string(map.id);
                if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) PLConfig::NextMapPos(map.id, Vector3());
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("NPCList"), ImVec2(0, 0), true);
            if (!PLConfig::npcMap.empty()) {
                for (const auto &p : PLConfig::npcMap) {
                    std::string btn = p.second.name + OBF("##") + std::to_string((uintptr_t) p.first);
                    if (ImGui::Button(btn.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) PLConfig::NextMapPos(0, p.second.pos);
                }
            } else {
                ImGui::TextUnformatted(OBF("Chưa có NPC"));
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Dịch chuyển"))) {
            ImGui::BeginChild(OBF("TeleList1"), ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0), true);
            struct TelePos { const char *name; int mapID; Vector3 pos; bool random; };
            TelePos presets[] = {
                {OBF("Bán cá"), 1001, {43.20f, -0.82f, -45.26f}, false},
                {OBF("Cầu cảng"), 1001, {120.0f, -0.5f, 80.0f}, true},
                {OBF("Khu cắm trại"), 1201, {0.f, 0.f, 0.f}, true},
            };
            for (int i = 0; i < 3; ++i) {
                TelePos &item = presets[i];
                std::string teleBtn = std::string(item.name) + OBF("##telepreset") + std::to_string(i);
                if (ImGui::Button(teleBtn.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                    float ran = item.random ? ((float) rand() / RAND_MAX * 2.f - 1.f) * 10.f : 0.f;
                    PLConfig::NextMapPos(item.mapID, {item.pos.x + ran, item.pos.y, item.pos.z + ran});
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("TeleList2"), ImVec2(0, 0), true);
            static char saveNameBuf[64] = {};
            ImGui::InputText(OBF("##SavePosName"), saveNameBuf, sizeof(saveNameBuf));
            ImGui::SameLine();
            if (ImGui::Button(OBF("Lưu vị trí##map_save_pos"))) {
                std::string key = saveNameBuf[0] ? saveNameBuf : OBF("Vị trí");
                key += std::to_string(gPLConfig.viTriCoSan.size());
                gPLConfig.viTriCoSan[key] = {saveNameBuf, PLConfig::GetPlayerMapID(), PLConfig::GetPlayerPosition()};
                saveNameBuf[0] = '\0';
                SaveConfig();
            }
            for (auto it = gPLConfig.viTriCoSan.begin(); it != gPLConfig.viTriCoSan.end();) {
                std::string gotoBtn = it->second.name + OBF("##savedpos") + it->first;
                if (ImGui::Button(gotoBtn.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 80.f, 0))) PLConfig::NextMapPos(it->second.mapID, it->second.pos);
                ImGui::SameLine();
                std::string del = OBF("Xóa##saved") + it->first;
                if (ImGui::Button(del.c_str(), ImVec2(70.f, 0))) {
                    it = gPLConfig.viTriCoSan.erase(it);
                    SaveConfig();
                    continue;
                }
                ++it;
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabSettings() {
    UiCheckbox(OBF("Sửa đồ"), &gPLConfig.general.isRepair);
    ImGui::Separator();
    UiCheckbox(OBF("Khôi phục TT"), &gPLConfig.general.isResetTrangThai);
    ImGui::Separator();
    if (ImGui::Button(OBF("Lưu##settings_save"), ImVec2(-1, 0))) SaveConfig();
    if (ImGui::Button(OBF("Tải##settings_load"), ImVec2(-1, 0))) LoadConfig();
    ImGui::Separator();
    ImGui::Text(OBF("Map: %d"), PLConfig::GetPlayerMapID());
    Vector3 pos = PLConfig::GetPlayerPosition();
    ImGui::Text(OBF("Tọa độ: %.1f, %.1f, %.1f"), pos.x, pos.y, pos.z);
}

}

void SetupMenuUi() {
    modui::AppUi ui{};
    ui.menu_size = ImVec2(720.f, 520.f);
    ui.fab_icon_path = OBF("/data/user/0/") + Tools::GetPackageName() + OBF("/files/fab.png");
    ui.set_window_title(OBF("Play Together - Câu Cá##modui_shell"));
    ui.add_tab(OBF("cauca"), OBF("Câu Cá"), DrawTabCauCa);
    ui.add_tab(OBF("map"), OBF("Bản Đồ"), DrawTabMap);
    ui.add_tab(OBF("settings"), OBF("Cài Đặt"), DrawTabSettings);
    modui::SetAppUi(ui);
}

}
