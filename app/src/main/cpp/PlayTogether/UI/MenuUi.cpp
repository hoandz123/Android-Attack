#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "FishLogger.h"
#include "Stubs/AutoCollect.h"
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
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
    if (ImGui::Button(OBF("Lam Moi"))) {
        logs = PLConfig::FishingConfig::gFishLogger.LoadHistory(dates);
        dateSel = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button(OBF("Xoa Lich Su"))) {
        PLConfig::FishingConfig::gFishLogger.Delete();
        logs = PLConfig::FishingConfig::gFishLogger.LoadHistory(dates);
        dateSel = 0;
        SaveConfig();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(OBF("Chon Ngay"));
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
                ImGui::TableSetupColumn(OBF("Ten"));
                ImGui::TableSetupColumn(OBF("ID"));
                ImGui::TableSetupColumn(OBF("Vung Cau"));
                ImGui::TableSetupColumn(OBF("Bong"));
                ImGui::TableSetupColumn(OBF("Thoi Gian"));
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
        drawTable(OBF("HistSuccess"), OBF("Da Cau"), true);
        drawTable(OBF("HistFailure"), OBF("Dut Day"), false);
        ImGui::EndTabBar();
    }
}

static void DrawTabChung() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild(OBF("ChungL"), ImVec2(avail.x * 0.5f, 0), true);
    UiCheckbox(OBF("Tu Dong Toi (chi duong)"), &gPLConfig.general.isTeleNpc);
    ImGui::Separator();
    UiCheckbox(OBF("Sua Dung Cu"), &gPLConfig.general.isRepair);
    ImGui::Separator();
    if (UiCheckbox(OBF("Bao Quan"), &gPLConfig.general.isBaoQuan) && gPLConfig.general.isBaoQuan) gPLConfig.general.isBanGoi = false;
    ImGui::Separator();
    if (UiCheckbox(OBF("Mo Hop Qua/Goi The"), &gPLConfig.general.isMoHopQua) && gPLConfig.general.isMoHopQua) gPLConfig.general.isMoHopQua2 = false;
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild(OBF("ChungR"), ImVec2(0, 0), true);
    if (ImGui::CollapsingHeader(OBF("Ban Nhanh"))) {
        if (UiCheckbox(OBF("Ban Nhanh"), &gPLConfig.general.isBanGoi) && gPLConfig.general.isBanGoi) gPLConfig.general.isBaoQuan = false;
        ImGui::Separator();
        if (UiCheckbox(OBF("Giu Nen Tim,VVIP"), &gPLConfig.general.isDuNenVip) && gPLConfig.general.isDuNenVip) gPLConfig.general.isDuB67 = false;
        ImGui::Separator();
        if (UiCheckbox(OBF("Giu Bong 6,7"), &gPLConfig.general.isDuB67) && gPLConfig.general.isDuB67) gPLConfig.general.isDuNenVip = false;
    }
    ImGui::Separator();
    UiCheckbox(OBF("Hien Bang Thong Tin"), &gPLConfig.general.isInfo);
    ImGui::Separator();
    UiCheckbox(OBF("Khoi Phuc Trang Thai"), &gPLConfig.general.isResetTrangThai);
    ImGui::Separator();
    UiCheckbox(OBF("Nhan Thanh Tich"), &gPLConfig.general.isNhanThanhTich);
    ImGui::EndChild();
}

static void DrawTabCauCa() {
    auto &fishing = gPLConfig.fishing;
    if (ImGui::BeginTabBar(OBF("##FishingTabs"))) {
        if (ImGui::BeginTabItem(OBF("Cau Loc"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("FishL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Cau Ca"), &fishing.isCauCa);
            ImGui::Separator();
            UiCheckbox(OBF("Kich Hoat Loc ID"), &fishing.isLocID);
            static char addIdBuf[128] = {};
            ImGui::InputText(OBF("##AddFishId"), addIdBuf, sizeof(addIdBuf));
            ImGui::SameLine();
            if (ImGui::Button(OBF("Them ID"))) {
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
                std::string label = std::string(OBF("ID ")) + std::to_string(it->first);
                UiCheckbox(label.c_str(), &it->second);
                ImGui::SameLine();
                std::string del = OBF("Xoa##") + std::to_string(it->first);
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
            if (UiCheckbox(OBF("Tat loc bong"), &isOff) && isOff) for (int i = 0; i < 7; i++) fishing.locBong[i] = false;
            for (int i = 0; i < 7; i++) {
                std::string lbl = OBF("Bong ") + std::to_string(i + 1);
                UiCheckbox(lbl.c_str(), &fishing.locBong[i]);
            }
            ImGui::Separator();
            const char *grades[] = {OBF("Tat"), OBF("F"), OBF("D"), OBF("C"), OBF("B"), OBF("A"), OBF("S")};
            ImGui::TextUnformatted(OBF("Roll Dong Can"));
            if (ImGui::BeginCombo(OBF("##gradeCombo"), grades[fishing.RollCapDo])) {
                for (int n = 0; n < 7; n++) {
                    bool selected = fishing.RollCapDo == n;
                    if (ImGui::Selectable(grades[n], selected)) {
                        fishing.RollCapDo = n;
                        SaveConfig();
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Fake Zone [Rui Ro]"))) {
            ImGui::TextColored(ImVec4(1.f, 0.3f, 0.2f, 1.f), "%s", OBF("CANH BAO: Fake zone co the gay ban/crash. Tu chiu trach nhiem."));
            if (UiCheckbox(OBF("Fake Rong (zone 503)"), &fishing.isFakeVR) && fishing.isFakeVR) fishing.isFishZone = false;
            UiSliderInt(OBF("ID Vung Cau"), &fishing.fishZone, 0, 9999);
            UiCheckbox(OBF("Kich Hoat Fake Zone"), &fishing.isFishZone);
            if (fishing.isFishZone && !fishing.isFakeVR && fishing.fishZone <= 0) ImGui::TextColored(ImVec4(1.f, 0.6f, 0.1f, 1.f), "%s", OBF("Can nhap ID vung cau > 0"));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lich Su"))) {
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
        if (ImGui::BeginTabItem(OBF("Dap Da"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("ColL"), ImVec2(avail.x * 0.5f, avail.y), true);
            if (UiCheckbox(OBF("Tu Dong Dap Da"), &collect.isAutoDapDa) && collect.isAutoDapDa) {
                collect.esp.isEnable = false;
                gPLConfig.insect.isAutoBatBo = false;
                gPLConfig.insect.esp.isEnable = false;
                gPLConfig.fishing.isCauCa = false;
                SaveConfig();
            }
            UiCheckbox(OBF("Nhat The"), &collect.isAutoNhatThe);
            UiCheckbox(OBF("Nhat Nguyen Lieu"), &collect.isAutoNguyenLieu);
            UiCheckbox(OBF("Tu Dong Doi Map"), &collect.isTeleMapCollect);
            ImGui::Separator();
            ImGui::TextUnformatted(OBF("Chon Loai:"));
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
            UiSliderInt(OBF("Toc Do Doi Map (ms)"), &collect.delayNextMap, 10000, 30000);
            UiSliderInt(OBF("Toc Do Dap Da (ms)"), &collect.delayDapDa, 500, 2000);
            ImGui::Separator();
            ImGui::TextUnformatted(OBF("Chon Map:"));
            for (const PLConfig::MapInfo &info : PLConfig::GetMapInfoList()) {
                auto it = collect.DSMapDa.find(info.id);
                if (it == collect.DSMapDa.end()) continue;
                std::string label = info.name + OBF("##") + std::to_string(info.id);
                UiCheckbox(label.c_str(), &it->second);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("ESP Collect"))) {
            auto &esp = collect.esp;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("EspColL"), ImVec2(avail.x * 0.5f, avail.y), true);
            if (UiCheckbox(OBF("Kich Hoat ESP"), &esp.isEnable) && esp.isEnable) {
                collect.isAutoDapDa = false;
                gPLConfig.insect.isAutoBatBo = false;
                gPLConfig.insect.esp.isEnable = false;
                SaveConfig();
            }
            if (esp.isEnable) {
                UiCheckbox(OBF("Hien Ten"), &esp.isShowName);
                UiCheckbox(OBF("Nut Teleport"), &esp.isTeleportButton);
                UiCheckbox(OBF("Mo quang"), &esp.isVein);
                UiCheckbox(OBF("Thuc vat"), &esp.isPlants);
                UiCheckbox(OBF("Hoa thach"), &esp.isFossil);
                UiCheckbox(OBF("Slime"), &esp.isSlime);
                UiCheckbox(OBF("Nguoi tuyet"), &esp.isSnowman);
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("EspColR"), ImVec2(0, avail.y), true);
            if (esp.isEnable) {
                UiCheckbox(OBF("Quang"), &esp.isOre);
                UiCheckbox(OBF("Nguyen lieu"), &esp.isIng);
                UiCheckbox(OBF("Vung cau ca"), &esp.isFishingZone);
                UiCheckbox(OBF("Khu thu thap"), &esp.isGathering);
                UiCheckbox(OBF("Thu the"), &esp.isCardCollect);
                UiCheckbox(OBF("Xu"), &esp.isCoin);
                UiCheckbox(OBF("The ten"), &esp.isNameTag);
                UiCheckbox(OBF("Shop Banh Ca"), &esp.isFishBreadShop);
                UiCheckbox(OBF("Quai Rong"), &esp.isDragonVillageMonster);
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
        if (ImGui::BeginTabItem(OBF("Con Trung"))) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("InsL"), ImVec2(avail.x * 0.5f, avail.y), true);
            if (UiCheckbox(OBF("Tu Dong Bat Con Trung"), &insect.isAutoBatBo) && insect.isAutoBatBo) {
                insect.esp.isEnable = false;
                gPLConfig.collect.isAutoDapDa = false;
                gPLConfig.fishing.isCauCa = false;
                SaveConfig();
            }
            UiCheckbox(OBF("Dong Bang Con Trung [Rui Ro]"), &insect.isFreezeCT);
            UiCheckbox(OBF("Tu Dong Doi Map"), &insect.isTeleMapBo);
            UiCheckbox(OBF("Tu Dong Ban Con Trung"), &insect.isBanBo);
            UiCheckbox(OBF("Bat Con Trung Tren Troi"), &insect.isBatBoTrenTroi);
            UiCheckbox(OBF("Giu >= Tim khi ban"), &insect.isDuTimTroLen);
            if (insect.isAutoBatBo) UiCheckbox(OBF("Nhat The"), &insect.isNhatThe);
            const int gradeValues[] = {-1, 0, 1, 2, 3, 4};
            const char *gradeLabels[] = {OBF("Khong loc"), OBF("Trang +"), OBF("Xanh La +"), OBF("Xanh Duong +"), OBF("Tim +"), OBF("VVIP")};
            int currentIdx = 0;
            for (int i = 0; i < 6; ++i) if (insect.minInsectGrade == gradeValues[i]) currentIdx = i;
            if (ImGui::BeginCombo(OBF("Loc cap con trung"), gradeLabels[currentIdx])) {
                for (int i = 0; i < 6; ++i) {
                    if (ImGui::Selectable(gradeLabels[i], insect.minInsectGrade == gradeValues[i])) {
                        insect.minInsectGrade = gradeValues[i];
                        SaveConfig();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::EndChild();
            ImGui::SameLine();
            ImGui::BeginChild(OBF("InsR"), ImVec2(0, avail.y), true);
            UiSliderInt(OBF("Toc Do Bat (ms)"), &insect.delayBatCT, 5000, 30000);
            UiSliderInt(OBF("Toc Do Doi Map (ms)"), &insect.delayTeleMap, 10000, 60000);
            UiSliderInt(OBF("So Luong Ban"), &insect.MaxInsectSell, 10, 250);
            UiSliderInt(OBF("Do Cao Con Trung"), &insect.conTrungCachMatDat, 1, 100);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Chon Map"))) {
            for (const PLConfig::MapInfo &info : PLConfig::GetMapInfoList()) {
                auto it = insect.DSMapBo.find(info.id);
                if (it == insect.DSMapBo.end()) continue;
                if (info.id == 1501) it->second = false;
                std::string label = info.name + OBF("##") + std::to_string(info.id);
                UiCheckbox(label.c_str(), &it->second);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("ESP Insect"))) {
            auto &esp = insect.esp;
            if (UiCheckbox(OBF("Kich Hoat ESP"), &esp.isEnable) && esp.isEnable) {
                insect.isAutoBatBo = false;
                gPLConfig.collect.isAutoDapDa = false;
                SaveConfig();
            }
            if (esp.isEnable) {
                UiCheckbox(OBF("Hien Ten"), &esp.isShowName);
                UiCheckbox(OBF("Nut Teleport"), &esp.isTeleportButton);
                const char *labels[] = {OBF("Trang"), OBF("Xanh La"), OBF("Xanh Duong"), OBF("Tim"), OBF("VVIP")};
                for (int i = 0; i < 5; i++) UiCheckbox(labels[i], &esp.isShowGrade[i]);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabEvent() {
    InitMonsterDefaults();
    if (ImGui::BeginTabBar(OBF("##EventTabs"))) {
        if (ImGui::BeginTabItem(OBF("Quai Vat"))) {
            auto &monster = gPLConfig.monster;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::BeginChild(OBF("MonL"), ImVec2(avail.x * 0.5f, avail.y), true);
            UiCheckbox(OBF("Kich Hoat"), &monster.isEnable);
            if (monster.isEnable) {
                UiCheckbox(OBF("Auto Ban Quai"), &monster.isAutoMonster);
                UiCheckbox(OBF("Nhat Phan Thuong"), &monster.isCollectReward);
                UiCheckbox(OBF("Tu Doi Map"), &monster.isTeleMapMonster);
                UiSliderInt(OBF("Toc Do Doi Map (ms)"), &monster.delayNextMap, 10000, 30000);
                UiSliderInt(OBF("Toc Do Ban (ms)"), &monster.tocDoBanQuaiVat, 500, 1000);
                UiSliderInt(OBF("HP Duoi"), &monster.banQuaiVatHpDuoi, 1000, 350000);
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
            UiCheckbox(OBF("ESP Quai"), &esp.isEnable);
            if (esp.isEnable) {
                UiCheckbox(OBF("Hien Ten"), &esp.isShowName);
                UiCheckbox(OBF("Nut Teleport"), &esp.isTeleportButton);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Nong Trai"))) {
            auto &farm = gPLConfig.farm;
            UiCheckbox(OBF("Tu Dong Click Thu"), &farm.isAutoClickCollect);
            UiCheckbox(OBF("Tu Dong Trong"), &farm.isAutoPlant);
            if (farm.isAutoPlant) {
                int seedId = (int) farm.selectedSeedId;
                if (UiSliderInt(OBF("Seed ID"), &seedId, 0, 999999)) farm.selectedSeedId = (uint32_t) seedId;
                UiSliderInt(OBF("Delay Trong (ms)"), &farm.delayPlant, 500, 5000);
            }
            UiCheckbox(OBF("Tu Dong Thu Hoach"), &farm.isAutoReap);
            if (farm.isAutoReap) UiSliderInt(OBF("Delay Thu (ms)"), &farm.delayReap, 500, 5000);
            auto &esp = farm.esp;
            ImGui::Separator();
            UiCheckbox(OBF("ESP Farm"), &esp.isEnable);
            if (esp.isEnable) {
                UiCheckbox(OBF("Hien Ten"), &esp.isShowName);
                UiCheckbox(OBF("Hien Loai"), &esp.isShowType);
                UiCheckbox(OBF("Nut Teleport"), &esp.isTeleportButton);
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabMap() {
    if (ImGui::BeginTabBar(OBF("##MapTabs"))) {
        if (ImGui::BeginTabItem(OBF("MAP"))) {
            ImGui::BeginChild(OBF("MapList"), ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0), true);
            for (const PLConfig::MapInfo &map : PLConfig::GetMapInfoList()) {
                std::string label = map.name;
                if (PLConfig::GetPlayerMapID() == map.id) label += OBF(" [Hien Tai]");
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
                ImGui::TextUnformatted(OBF("Chua co du lieu NPC"));
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Dich Chuyen"))) {
            ImGui::BeginChild(OBF("TeleList1"), ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0), true);
            struct TelePos { const char *name; int mapID; Vector3 pos; bool random; };
            TelePos presets[] = {
                {OBF("Xa Khoi 1"), 1301, {323.48f, -0.28f, 499.77f}, true},
                {OBF("Ban Ca"), 1001, {43.20f, -0.82f, -45.26f}, false},
                {OBF("Ban Con Trung"), 1201, {-1.85f, 0.001f, -17.60f}, false},
            };
            for (int i = 0; i < 3; ++i) {
                TelePos &item = presets[i];
                if (ImGui::Button(item.name, ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
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
            if (ImGui::Button(OBF("Luu Vi Tri"))) {
                std::string key = saveNameBuf[0] ? saveNameBuf : OBF("Vi Tri");
                key += std::to_string(gPLConfig.viTriCoSan.size());
                gPLConfig.viTriCoSan[key] = {saveNameBuf, PLConfig::GetPlayerMapID(), PLConfig::GetPlayerPosition()};
                saveNameBuf[0] = '\0';
                SaveConfig();
            }
            for (auto it = gPLConfig.viTriCoSan.begin(); it != gPLConfig.viTriCoSan.end();) {
                if (ImGui::Button(it->second.name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 80.f, 0))) PLConfig::NextMapPos(it->second.mapID, it->second.pos);
                ImGui::SameLine();
                std::string del = OBF("Xoa##") + it->first;
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
            UiCheckbox(OBF("Kich Hoat"), &z.isEnable);
            if (z.isEnable) {
                UiCheckbox(OBF("Chem Xa"), &z.isChemXa);
                UiCheckbox(OBF("ESP"), &z.isEsp);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Kho Bau"))) {
            auto &d = gPLConfig.miniGame.digging;
            UiCheckbox(OBF("Kich Hoat"), &d.isEnable);
            if (d.isEnable) {
                UiCheckbox(OBF("Tu Dong Dao Ruong"), &d.isAutoDigTreasure);
                if (d.isAutoDigTreasure) {
                    UiSliderInt(OBF("Cap Do An Toan"), &d.capDoAnToan, 1, 10);
                    UiSliderFloat(OBF("Goc Lech"), &d.gocLechToiDa, 0.f, 70.f, "%.0f");
                    UiSliderFloat(OBF("Chu Ky S"), &d.chuKyChuS, 0.f, 30.f, "%.0f");
                    UiSliderFloat(OBF("Do Cong S"), &d.doCongChuS, 0.f, 10.f, "%.1f");
                    UiSliderFloat(OBF("Khoang Cach Cham"), &d.khoangCachBatDauCham, 3.f, 50.f, "%.0f");
                    UiSliderFloat(OBF("Ban Kinh Tim"), &d.radiusTim, 10.f, 300.f, "%.0f");
                    UiSliderFloat(OBF("Khoang Cach Max"), &d.maxKhoangCach, 20.f, 500.f, "%.0f");
                    UiCheckbox(OBF("Di Ngau Nhien Het Ruong"), &d.autoMoveKhiHetRuong);
                }
                UiCheckbox(OBF("Tu Mua Xeng"), &d.isAutoBuyXeng);
                UiCheckbox(OBF("ESP Kho Bau"), &d.esp.isEnable);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Leo Thap"))) {
            auto &t = gPLConfig.miniGame.towerClimb;
            UiCheckbox(OBF("Kich Hoat"), &t.isEnable);
            if (t.isEnable) UiSliderInt(OBF("Delay Step (ms)"), &t.delayNextPoint, 8000, 30000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Obby"))) {
            auto &o = gPLConfig.miniGame.obby;
            UiCheckbox(OBF("Kich Hoat"), &o.isEnable);
            if (o.isEnable) UiSliderInt(OBF("Delay Step (ms)"), &o.delayNextPoint, 5000, 30000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Sky High"))) {
            auto &s = gPLConfig.miniGame.ThapGa;
            UiCheckbox(OBF("Kich Hoat"), &s.isEnable);
            if (s.isEnable) UiSliderInt(OBF("Delay Step (ms)"), &s.delayNextPoint, 5000, 30000);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

static void DrawTabSettings() {
    if (ImGui::Button(OBF("Luu Cau Hinh"), ImVec2(-1, 0))) SaveConfig();
    if (ImGui::Button(OBF("Tai Cau Hinh"), ImVec2(-1, 0))) LoadConfig();
    ImGui::Separator();
    ImGui::Text(OBF("Map hien tai: %d"), PLConfig::GetPlayerMapID());
    Vector3 pos = PLConfig::GetPlayerPosition();
    ImGui::Text(OBF("Vi tri: %.1f, %.1f, %.1f"), pos.x, pos.y, pos.z);
}

}

void SetupMenuUi() {
    modui::AppUi ui{};
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("chung"), OBF("Chung"), DrawTabChung);
    ui.add_tab(OBF("cauca"), OBF("Cau Ca"), DrawTabCauCa);
    ui.add_tab(OBF("collect"), OBF("Collect"), DrawTabCollect);
    ui.add_tab(OBF("insect"), OBF("Con Trung"), DrawTabInsect);
    ui.add_tab(OBF("event"), OBF("Su Kien"), DrawTabEvent);
    ui.add_tab(OBF("map"), OBF("Map"), DrawTabMap);
    ui.add_tab(OBF("minigame"), OBF("MiniGame"), DrawTabMiniGame);
    ui.add_tab(OBF("settings"), OBF("Cai Dat"), DrawTabSettings);
    modui::SetAppUi(ui);
}

}
