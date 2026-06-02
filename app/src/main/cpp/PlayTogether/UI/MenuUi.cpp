#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "FishLogger.h"
#include "Stubs/AutoCollect.h"
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>
#include <imgui.h>
#include <algorithm>
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

static bool UiSliderFloat(const char *label, float *v, float minV, float maxV, const char *fmt) {
    bool changed = ImGui::SliderFloat(label, v, minV, maxV, fmt);
    if (changed) SaveConfig();
    return changed;
}

static void InitCollectDefaults() {
    if (gPLConfig.collect.DSTypeDa.empty()) {
        CollectSys::SpawnType initTypes[] = {CollectSys::SpawnType::Vein, CollectSys::SpawnType::GemVein, CollectSys::SpawnType::Ore};
        for (CollectSys::SpawnType t : initTypes) gPLConfig.collect.DSTypeDa[t] = true;
    }
    if (gPLConfig.collect.DSMapDa.empty()) {
        int collectMaps[] = {1001, 1201, 1502};
        for (int id : collectMaps) gPLConfig.collect.DSMapDa[id] = false;
    }
}

static void InitMonsterDefaults() {
    if (gPLConfig.monster.DSMapMonster.empty()) {
        int monsterMaps[] = {1001, 1201, 1502};
        for (int id : monsterMaps) gPLConfig.monster.DSMapMonster[id] = true;
    }
}

static void InitInsectMapDefaults() {
    if (gPLConfig.insect.DSMapBo.empty()) {
        gPLConfig.insect.DSMapBo = {{1001, true}, {1201, true}, {1501, false}, {1502, false}, {1301, false}, {1401, false}};
    }
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

static void DrawTabChung() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild(OBF("ChungL"), ImVec2(avail.x * 0.5f, 0), true);
    UiCheckbox(OBF("Tự đến NPC"), &gPLConfig.general.isTeleNpc);
    ImGui::Separator();
    UiCheckbox(OBF("Sửa đồ"), &gPLConfig.general.isRepair);
    ImGui::Separator();
    if (UiCheckbox(OBF("Bảo quản"), &gPLConfig.general.isBaoQuan) && gPLConfig.general.isBaoQuan) gPLConfig.general.isBanGoi = false;
    ImGui::Separator();
    if (UiCheckbox(OBF("Mở hộp/gói"), &gPLConfig.general.isMoHopQua) && gPLConfig.general.isMoHopQua) gPLConfig.general.isMoHopQua2 = false;
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild(OBF("ChungR"), ImVec2(0, 0), true);
    if (ImGui::CollapsingHeader(OBF("Bán nhanh##hdr_ban_nhanh"))) {
        if (UiCheckbox(OBF("Bán nhanh##chk_ban_nhanh"), &gPLConfig.general.isBanGoi) && gPLConfig.general.isBanGoi) gPLConfig.general.isBaoQuan = false;
        ImGui::Separator();
        if (UiCheckbox(OBF("Giữ Tím+VVIP"), &gPLConfig.general.isDuNenVip) && gPLConfig.general.isDuNenVip) gPLConfig.general.isDuB67 = false;
        ImGui::Separator();
        if (UiCheckbox(OBF("Giữ bóng 6–7"), &gPLConfig.general.isDuB67) && gPLConfig.general.isDuB67) gPLConfig.general.isDuNenVip = false;
    }
    ImGui::Separator();
    UiCheckbox(OBF("Bảng thông tin"), &gPLConfig.general.isInfo);
    ImGui::Separator();
    UiCheckbox(OBF("Khôi phục TT"), &gPLConfig.general.isResetTrangThai);
    ImGui::Separator();
    UiCheckbox(OBF("Nhận thành tích"), &gPLConfig.general.isNhanThanhTich);
    ImGui::Separator();
    UiCheckbox(OBF("Nhiệm vụ ngày"), &gPLConfig.general.isNhanNhiemVuNgay);
    ImGui::Separator();
    UiCheckbox(OBF("Tem ngày"), &gPLConfig.general.isNhanTemNgay);
    ImGui::Separator();
    UiCheckbox(OBF("Nhận thư"), &gPLConfig.general.isNhanThu);
    ImGui::EndChild();
}

static void DrawTabCauCa() {
    auto &fishing = gPLConfig.fishing;
    if (ImGui::BeginTabBar(OBF("##FishingTabs"))) {
        if (ImGui::BeginTabItem(OBF("Lọc câu"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("FishL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Câu cá"), &fishing.isCauCa);
            ImGui::Separator();
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
        if (ImGui::BeginTabItem(OBF("Bổ trợ"))) {
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
            if (fishing.isFishZone && !fishing.isFakeVR && fishing.fishZone <= 0) ImGui::TextColored(ImVec4(1.f, 0.6f, 0.1f, 1.f), "%s", OBF("Cần ID vùng > 0"));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lịch sử"))) {
            ShowFishingHistory();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabCollect() {
    InitCollectDefaults();
    auto &collect = gPLConfig.collect;
    if (ImGui::BeginTabBar(OBF("##CollectTabs"))) {
        if (ImGui::BeginTabItem(OBF("Đập đá"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("ColL"), ImVec2(avail.x * 0.5f, avail.y), true);
            if (UiCheckbox(OBF("Tự đập đá"), &collect.isAutoDapDa) && collect.isAutoDapDa) {
                collect.esp.isEnable = false;
                gPLConfig.insect.isAutoBatBo = false;
                gPLConfig.insect.esp.isEnable = false;
                gPLConfig.fishing.isCauCa = false;
                SaveConfig();
            }
            UiCheckbox(OBF("Nhặt thẻ"), &collect.isAutoNhatThe);
            UiCheckbox(OBF("Nhặt NL"), &collect.isAutoNguyenLieu);
            UiCheckbox(OBF("Tự đổi map"), &collect.isTeleMapCollect);
            ImGui::Separator();
            ImGui::TextUnformatted(OBF("Loại:"));
            for (int i = 0; i <= (int) CollectSys::SpawnType::DragonVillageMonster; ++i) {
                CollectSys::SpawnType type = (CollectSys::SpawnType) i;
                auto it = collect.DSTypeDa.find(type);
                if (it == collect.DSTypeDa.end()) continue;
                std::string label = CollectSys::GetSpawnTypeName(type) + OBF("##") + std::to_string(i);
                UiCheckbox(label.c_str(), &it->second);
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("ColR"), ImVec2(0, avail.y), true);
            UiSliderInt(OBF("Đổi map (ms)"), &collect.delayNextMap, 10000, 30000);
            UiSliderInt(OBF("Đập đá (ms)"), &collect.delayDapDa, 500, 2000);
            ImGui::Separator();
            ImGui::TextUnformatted(OBF("Map:"));
            for (const PLConfig::MapInfo &info : PLConfig::GetMapInfoList()) {
                auto it = collect.DSMapDa.find(info.id);
                if (it == collect.DSMapDa.end()) continue;
                std::string label = info.name + OBF("##") + std::to_string(info.id);
                UiCheckbox(label.c_str(), &it->second);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("ESP thu thập"))) {
            auto &esp = collect.esp;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("EspColL"), ImVec2(avail.x * 0.5f, avail.y), true);
            if (UiCheckbox(OBF("Bật ESP##esp_collect_l"), &esp.isEnable) && esp.isEnable) {
                collect.isAutoDapDa = false;
                gPLConfig.insect.isAutoBatBo = false;
                gPLConfig.insect.esp.isEnable = false;
                SaveConfig();
            }
            if (esp.isEnable) {
                UiCheckbox(OBF("Tên##esp_collect_l"), &esp.isShowName);
                UiCheckbox(OBF("Nút tele##esp_collect_l"), &esp.isTeleportButton);
                UiCheckbox(OBF("Mạch##esp_collect_l"), &esp.isVein);
                UiCheckbox(OBF("Thực vật##esp_collect_l"), &esp.isPlants);
                UiCheckbox(OBF("Hóa thạch##esp_collect_l"), &esp.isFossil);
                UiCheckbox(OBF("Slime##esp_collect_l"), &esp.isSlime);
                UiCheckbox(OBF("Người tuyết##esp_collect_l"), &esp.isSnowman);
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("EspColR"), ImVec2(0, avail.y), true);
            if (esp.isEnable) {
                UiCheckbox(OBF("Quặng##esp_collect_r"), &esp.isOre);
                UiCheckbox(OBF("Nguyên liệu##esp_collect_r"), &esp.isIng);
                UiCheckbox(OBF("Khu câu##esp_collect_r"), &esp.isFishingZone);
                UiCheckbox(OBF("Thu thập##esp_collect_r"), &esp.isGathering);
                UiCheckbox(OBF("Thẻ##esp_collect_r"), &esp.isCardCollect);
                UiCheckbox(OBF("Xu##esp_collect_r"), &esp.isCoin);
                UiCheckbox(OBF("Thẻ tên##esp_collect_r"), &esp.isNameTag);
                UiCheckbox(OBF("Tiệm bánh##esp_collect_r"), &esp.isFishBreadShop);
                UiCheckbox(OBF("Quái rồng##esp_collect_r"), &esp.isDragonVillageMonster);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabInsect() {
    InitInsectMapDefaults();
    auto &insect = gPLConfig.insect;
    if (ImGui::BeginTabBar(OBF("##InsectTabs"))) {
        if (ImGui::BeginTabItem(OBF("Bắt bò"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("InsL"), ImVec2(avail.x * 0.5f, avail.y), true);
            if (UiCheckbox(OBF("Tự bắt bò"), &insect.isAutoBatBo) && insect.isAutoBatBo) {
                insect.esp.isEnable = false;
                gPLConfig.collect.isAutoDapDa = false;
                gPLConfig.fishing.isCauCa = false;
                SaveConfig();
            }
            UiCheckbox(OBF("Đóng băng [⚠]"), &insect.isFreezeCT);
            UiCheckbox(OBF("Tự đổi map"), &insect.isTeleMapBo);
            UiCheckbox(OBF("Tự bán bò"), &insect.isBanBo);
            UiCheckbox(OBF("Bắt trên trời"), &insect.isBatBoTrenTroi);
            UiCheckbox(OBF("Giữ Tím+"), &insect.isDuTimTroLen);
            if (insect.isAutoBatBo) UiCheckbox(OBF("Nhặt thẻ"), &insect.isNhatThe);
            const int gradeValues[] = {-1, 0, 1, 2, 3, 4};
            const char *gradeLabels[] = {OBF("Không lọc"), OBF("Trắng+"), OBF("X.lá+"), OBF("X.dương+"), OBF("Tím+"), OBF("VVIP")};
            int currentIdx = 0;
            for (int i = 0; i < 6; ++i) if (insect.minInsectGrade == gradeValues[i]) currentIdx = i;
            if (ImGui::BeginCombo(OBF("Lọc cấp##insect_grade"), gradeLabels[currentIdx])) {
                for (int i = 0; i < 6; ++i) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(gradeLabels[i], insect.minInsectGrade == gradeValues[i])) {
                        insect.minInsectGrade = gradeValues[i];
                        SaveConfig();
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("InsR"), ImVec2(0, avail.y), true);
            UiSliderInt(OBF("Bắt (ms)"), &insect.delayBatCT, 5000, 30000);
            UiSliderInt(OBF("Đổi map (ms)"), &insect.delayTeleMap, 10000, 60000);
            UiSliderInt(OBF("SL bán"), &insect.MaxInsectSell, 10, 250);
            UiSliderInt(OBF("Độ cao"), &insect.conTrungCachMatDat, 1, 100);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Chọn map"))) {
            for (const PLConfig::MapInfo &info : PLConfig::GetMapInfoList()) {
                auto it = insect.DSMapBo.find(info.id);
                if (it == insect.DSMapBo.end()) continue;
                if (info.id == 1501) it->second = false;
                std::string label = info.name + OBF("##") + std::to_string(info.id);
                UiCheckbox(label.c_str(), &it->second);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("ESP bò"))) {
            auto &esp = insect.esp;
            if (UiCheckbox(OBF("Bật ESP##esp_insect"), &esp.isEnable) && esp.isEnable) {
                insect.isAutoBatBo = false;
                gPLConfig.collect.isAutoDapDa = false;
                SaveConfig();
            }
            if (esp.isEnable) {
                UiCheckbox(OBF("Tên##esp_insect"), &esp.isShowName);
                UiCheckbox(OBF("Nút tele##esp_insect"), &esp.isTeleportButton);
                const char *labels[] = {OBF("Trắng"), OBF("X.lá"), OBF("X.dương"), OBF("Tím"), OBF("VVIP")};
                for (int i = 0; i < 5; i++) {
                    ImGui::PushID(i);
                    UiCheckbox(labels[i], &esp.isShowGrade[i]);
                    ImGui::PopID();
                }
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabEvent() {
    InitMonsterDefaults();
    if (ImGui::BeginTabBar(OBF("##EventTabs"))) {
        if (ImGui::BeginTabItem(OBF("Quái vật"))) {
            auto &monster = gPLConfig.monster;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("MonL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Bật"), &monster.isEnable);
            if (monster.isEnable) {
                UiCheckbox(OBF("Tự bán"), &monster.isAutoMonster);
                UiCheckbox(OBF("Nhặt thưởng"), &monster.isCollectReward);
                UiCheckbox(OBF("Đổi map"), &monster.isTeleMapMonster);
                UiSliderInt(OBF("Đổi map (ms)"), &monster.delayNextMap, 10000, 30000);
                UiSliderInt(OBF("Bán (ms)"), &monster.tocDoBanQuaiVat, 500, 1000);
                UiSliderInt(OBF("HP dưới"), &monster.banQuaiVatHpDuoi, 1000, 350000);
                ImGui::Separator();
                for (const PLConfig::MapInfo &info : PLConfig::GetMapInfoList()) {
                    auto it = monster.DSMapMonster.find(info.id);
                    if (it == monster.DSMapMonster.end()) continue;
                    std::string label = info.name + OBF("##") + std::to_string(info.id);
                    UiCheckbox(label.c_str(), &it->second);
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("MonR"), ImVec2(0, avail.y), true);
            auto &esp = monster.esp;
            UiCheckbox(OBF("ESP quái"), &esp.isEnable);
            if (esp.isEnable) {
                UiCheckbox(OBF("Tên##esp_monster"), &esp.isShowName);
                UiCheckbox(OBF("Nút tele##esp_monster"), &esp.isTeleportButton);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Nông trại"))) {
            auto &farm = gPLConfig.farm;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("FarmL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Tự click thu"), &farm.isAutoClickCollect);
            ImGui::Separator();
            UiCheckbox(OBF("Tự trồng"), &farm.isAutoPlant);
            if (farm.isAutoPlant) {
                int seedId = (int) farm.selectedSeedId;
                if (UiSliderInt(OBF("ID hạt"), &seedId, 0, 999999)) farm.selectedSeedId = (uint32_t) seedId;
                UiSliderInt(OBF("Trồng (ms)"), &farm.delayPlant, 500, 5000);
                ImGui::TextUnformatted(OBF("Ô đất (trống=tất cả):"));
                for (int i = 0; i < 50; i++) {
                    bool checked = false;
                    auto plotIt = farm.selectedPlotPositions.find(i);
                    if (plotIt != farm.selectedPlotPositions.end()) checked = plotIt->second;
                    std::string plotLbl = OBF("Ô ") + std::to_string(i) + OBF("##plot") + std::to_string(i);
                    if (ImGui::Checkbox(plotLbl.c_str(), &checked)) {
                        if (checked) farm.selectedPlotPositions[i] = true;
                        else farm.selectedPlotPositions.erase(i);
                        SaveConfig();
                    }
                }
            }
            ImGui::Separator();
            UiCheckbox(OBF("Tự thu hoạch"), &farm.isAutoReap);
            if (farm.isAutoReap) {
                UiSliderInt(OBF("Thu (ms)"), &farm.delayReap, 500, 5000);
                ImGui::TextUnformatted(OBF("Loại cây:"));
                static char addCropBuf[64] = {};
                ImGui::InputText(OBF("##AddCropId"), addCropBuf, sizeof(addCropBuf));
                ImGui::SameLine();
                if (ImGui::Button(OBF("Thêm cây##farm_add_crop"))) {
                    try {
                        uint32_t cropId = (uint32_t) std::stoul(addCropBuf);
                        farm.selectedCropTypes[cropId] = true;
                        addCropBuf[0] = '\0';
                        SaveConfig();
                    } catch (...) {}
                }
                for (auto it = farm.selectedCropTypes.begin(); it != farm.selectedCropTypes.end();) {
                    std::string cropLbl = OBF("Cây ") + std::to_string(it->first) + OBF("##crop") + std::to_string(it->first);
                    UiCheckbox(cropLbl.c_str(), &it->second);
                    ImGui::SameLine();
                    std::string delCrop = OBF("Xóa##crop") + std::to_string(it->first);
                    if (ImGui::Button(delCrop.c_str())) {
                        it = farm.selectedCropTypes.erase(it);
                        SaveConfig();
                        continue;
                    }
                    ++it;
                }
                ImGui::TextUnformatted(OBF("Ô thu:"));
                for (int i = 0; i < 50; i++) {
                    bool checked = false;
                    auto reapIt = farm.selectedReapPositions.find(i);
                    if (reapIt != farm.selectedReapPositions.end()) checked = reapIt->second;
                    std::string reapLbl = OBF("Ô ") + std::to_string(i) + OBF("##reap") + std::to_string(i);
                    if (ImGui::Checkbox(reapLbl.c_str(), &checked)) {
                        if (checked) farm.selectedReapPositions[i] = true;
                        else farm.selectedReapPositions.erase(i);
                        SaveConfig();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("FarmR"), ImVec2(0, avail.y), true);
            auto &esp = farm.esp;
            UiCheckbox(OBF("ESP farm"), &esp.isEnable);
            if (esp.isEnable) {
                UiCheckbox(OBF("Tên##esp_farm"), &esp.isShowName);
                UiCheckbox(OBF("Loại##esp_farm"), &esp.isShowType);
                UiCheckbox(OBF("Nút tele##esp_farm"), &esp.isTeleportButton);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lưới AFK"))) {
            auto &ac = gPLConfig.autoCatch;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("AutoCatchL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Tự lưới/cần"), &ac.isAuto);
            if (ac.isAuto) {
                UiCheckbox(OBF("Thu hoạch"), &ac.isRetrieve);
                UiCheckbox(OBF("Kiểm tra"), &ac.isCheck);
                UiCheckbox(OBF("Lắp đặt"), &ac.isInstall);
                if (ac.isInstall) {
                    int mainId = (int) ac.mainItemId;
                    int subId = (int) ac.subItemId;
                    if (UiSliderInt(OBF("ID lưới/cần"), &mainId, 0, 999999)) ac.mainItemId = (uint32_t) mainId;
                    if (UiSliderInt(OBF("ID mồi"), &subId, 0, 999999)) ac.subItemId = (uint32_t) subId;
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("AutoCatchR"), ImVec2(0, avail.y), true);
            ImGui::TextUnformatted(OBF("ESP vùng: chưa hỗ trợ"));
            ImGui::EndChild();
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
                {OBF("Xa khối 1"), 1301, {323.48f, -0.28f, 499.77f}, true},
                {OBF("Bán cá"), 1001, {43.20f, -0.82f, -45.26f}, false},
                {OBF("Bán bò"), 1201, {-1.85f, 0.001f, -17.60f}, false},
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

static void DrawTabMiniGame() {
    if (ImGui::BeginTabBar(OBF("##MiniGameTabs"))) {
        if (ImGui::BeginTabItem(OBF("Zombie"))) {
            auto &z = gPLConfig.miniGame.zombie;
            UiCheckbox(OBF("Bật##zombie"), &z.isEnable);
            if (z.isEnable) {
                UiCheckbox(OBF("Chém xa##zombie"), &z.isChemXa);
                UiCheckbox(OBF("ESP##zombie"), &z.isEsp);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Kho báu"))) {
            auto &d = gPLConfig.miniGame.digging;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("DigL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Bật##digging"), &d.isEnable);
            if (d.isEnable) {
                UiCheckbox(OBF("Tự đào##digging"), &d.isAutoDigTreasure);
                if (d.isAutoDigTreasure) {
                    UiSliderInt(OBF("Cấp an toàn"), &d.capDoAnToan, 1, 10);
                    UiSliderFloat(OBF("Góc lệch"), &d.gocLechToiDa, 0.f, 70.f, "%.0f");
                    UiSliderFloat(OBF("Chu kỳ S"), &d.chuKyChuS, 0.f, 30.f, "%.0f");
                    UiSliderFloat(OBF("Độ cong S"), &d.doCongChuS, 0.f, 10.f, "%.1f");
                    UiSliderFloat(OBF("KC chạm"), &d.khoangCachBatDauCham, 3.f, 50.f, "%.0f");
                    UiSliderFloat(OBF("Bán kính"), &d.radiusTim, 10.f, 300.f, "%.0f");
                    UiSliderFloat(OBF("KC max"), &d.maxKhoangCach, 20.f, 500.f, "%.0f");
                    UiCheckbox(OBF("Đi random hết"), &d.autoMoveKhiHetRuong);
                }
                UiCheckbox(OBF("Tự mua xẻng"), &d.isAutoBuyXeng);
                ImGui::Separator();
                ImGui::TextUnformatted(OBF("Lọc rương:"));
                static char addBoxTypeBuf[64] = {};
                ImGui::InputText(OBF("##AddBoxType"), addBoxTypeBuf, sizeof(addBoxTypeBuf));
                ImGui::SameLine();
                if (ImGui::Button(OBF("Thêm loại##dig_add_box"))) {
                    try {
                        int boxType = std::stoi(addBoxTypeBuf);
                        d.filterLoaiRuong[boxType] = true;
                        addBoxTypeBuf[0] = '\0';
                        SaveConfig();
                    } catch (...) {}
                }
                for (auto it = d.filterLoaiRuong.begin(); it != d.filterLoaiRuong.end();) {
                    std::string boxLbl = OBF("Loại ") + std::to_string(it->first) + OBF("##box") + std::to_string(it->first);
                    UiCheckbox(boxLbl.c_str(), &it->second);
                    ImGui::SameLine();
                    std::string delBox = OBF("Xóa##box") + std::to_string(it->first);
                    if (ImGui::Button(delBox.c_str())) {
                        it = d.filterLoaiRuong.erase(it);
                        SaveConfig();
                        continue;
                    }
                    ++it;
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("DigR"), ImVec2(0, avail.y), true);
            if (d.isEnable) UiCheckbox(OBF("ESP kho"), &d.esp.isEnable);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Leo tháp"))) {
            auto &t = gPLConfig.miniGame.towerClimb;
            UiCheckbox(OBF("Bật##tower"), &t.isEnable);
            if (t.isEnable) UiSliderInt(OBF("Bước (ms)##tower"), &t.delayNextPoint, 8000, 30000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Obby"))) {
            auto &o = gPLConfig.miniGame.obby;
            UiCheckbox(OBF("Bật##obby"), &o.isEnable);
            if (o.isEnable) UiSliderInt(OBF("Bước (ms)##obby"), &o.delayNextPoint, 5000, 30000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Tháp gà"))) {
            auto &s = gPLConfig.miniGame.ThapGa;
            UiCheckbox(OBF("Bật##thapga"), &s.isEnable);
            if (s.isEnable) UiSliderInt(OBF("Bước (ms)##thapga"), &s.delayNextPoint, 5000, 30000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Party"))) {
            auto &party = gPLConfig.miniGame.Party;
            UiCheckbox(OBF("Bật##party"), &party.isEnable);
            if (party.isEnable) UiSliderInt(OBF("Bước (ms)##party"), &party.delayNextPoint, 5000, 30000);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabSettings() {
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
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("chung"), OBF("Chung"), DrawTabChung);
    ui.add_tab(OBF("cauca"), OBF("Câu Cá"), DrawTabCauCa);
    ui.add_tab(OBF("collect"), OBF("Thu Thập"), DrawTabCollect);
    ui.add_tab(OBF("insect"), OBF("Côn Trùng"), DrawTabInsect);
    ui.add_tab(OBF("event"), OBF("Sự Kiện"), DrawTabEvent);
    ui.add_tab(OBF("map"), OBF("Bản Đồ"), DrawTabMap);
    ui.add_tab(OBF("minigame"), OBF("Mini Game"), DrawTabMiniGame);
    ui.add_tab(OBF("settings"), OBF("Cài Đặt"), DrawTabSettings);
    modui::SetAppUi(ui);
}

}
