#include "InfoWindow.h"
#include "Config/Config.h"
#include "OverlaySnapshot.h"
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
        OverlaySnapshot::View snap{};
        OverlaySnapshot::Read(snap);
        if (snap.ready) {
            ImGui::Separator();
            ImGui::Text(OBF("Map: %d"), snap.mapId);
            ImGui::Text(OBF("Tọa độ: %.1f, %.1f, %.1f"), snap.position.x, snap.position.y, snap.position.z);
            if (gPLConfig.fishing.enabled && gPLConfig.fishing.showStatus) {
                ImGui::Separator();
                ImGui::Text(OBF("Câu cá: %s"), OverlaySnapshot::FishingStateLabel(snap.fishingState));
                ImGui::Text(OBF("Đã câu: %d"), snap.fishCaught);
                if (gPLConfig.fishing.showSessionStats) {
                    ImGui::Text(OBF("Phiên: %um — hụt %d / trượt %d"), snap.sessionSec / 60, snap.failCount, snap.missCount);
                    ImGui::Text(OBF("Theo grade: %d/%d/%d/%d/%d"), snap.catchGrade1, snap.catchGrade2, snap.catchGrade3, snap.catchGrade4, snap.catchGrade5);
                }
                if (snap.hasFloatPoint) ImGui::Text(OBF("Phao: %.1fm (Δx %.1f)"), snap.floatDistance, snap.floatOffsetX);
            }
        }
    }
    ImGui::End();
}
