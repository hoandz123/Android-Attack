#include "InfoWindow.h"
#include "Config/Config.h"
#include <Includes/obfuscate.h>
#include <imgui.h>
#include <chrono>

static auto g_infoStartTime = std::chrono::steady_clock::now();

void ShowInfoWindow() {
    if (!gPLConfig.general.isInfo) return;
    ImGui::SetNextWindowBgAlpha(0.88f);
    if (ImGui::Begin(OBF("Thông tin##info_overlay"), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        auto currentTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - g_infoStartTime);
        long long hours = duration.count() / 3600;
        long long minutes = (duration.count() % 3600) / 60;
        long long seconds = duration.count() % 60;
        char uptime[32];
        snprintf(uptime, sizeof(uptime), OBF("%02lld:%02lld:%02lld"), hours, minutes, seconds);
        ImGui::Text("%s %s", OBF("Uptime:"), uptime);
        ImGui::Separator();
        ImGui::Text(OBF("Map: %d"), PLConfig::GetPlayerMapID());
        Vector3 pos = PLConfig::GetPlayerPosition();
        ImGui::Text(OBF("Tọa độ: %.1f, %.1f, %.1f"), pos.x, pos.y, pos.z);
    }
    ImGui::End();
}
