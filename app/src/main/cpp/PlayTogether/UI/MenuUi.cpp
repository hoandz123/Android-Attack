#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>
#include <imgui.h>
#include <string>

namespace playtogether {

namespace {

static bool UiCheckbox(const char *label, bool *v) {
    bool changed = ImGui::Checkbox(label, v);
    if (changed) SaveConfig();
    return changed;
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
                {OBF("Quảng trường"), 1001, {43.20f, -0.82f, -45.26f}, false},
                {OBF("Khu cắm trại"), 1201, {0.f, 0.f, 0.f}, true},
            };
            for (int i = 0; i < 2; ++i) {
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
    UiCheckbox(OBF("Bảng thông tin"), &gPLConfig.general.isInfo);
    ImGui::Separator();
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
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("map"), OBF("Bản Đồ"), DrawTabMap);
    ui.add_tab(OBF("settings"), OBF("Cài Đặt"), DrawTabSettings);
    modui::SetAppUi(ui);
}

}
