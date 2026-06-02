#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "OverlaySnapshot.h"
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

static void DrawTabFishing() {
    ImGui::PushID(OBF("fishing_tab"));
    UiCheckbox(OBF("Bật câu cá tự động"), &gPLConfig.fishing.enabled);
    ImGui::Separator();
    UiCheckbox(OBF("Đóng hộp thưởng"), &gPLConfig.fishing.autoCloseReward);
    UiCheckbox(OBF("Hiện trạng thái"), &gPLConfig.fishing.showStatus);
    ImGui::Separator();
    if (ImGui::Checkbox(OBF("Cá lớn / raid (rủi ro)"), &gPLConfig.fishing.handleBigFish)) SaveConfig();
    if (gPLConfig.fishing.handleBigFish) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
        ImGui::TextUnformatted(OBF("Cảnh báo: dễ lệch server, mặc định tắt"));
        ImGui::PopStyleColor();
    }
    ImGui::Separator();
    ImGui::SliderInt(OBF("Nhịp tick (ms)##fish_tick"), &gPLConfig.fishing.tickIntervalMs, 250, 1200);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::SliderInt(OBF("Nhịp thao tác (ms)##fish_act"), &gPLConfig.fishing.actionIntervalMs, 300, 2000);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::SliderInt(OBF("Chờ cast lại (ms)##fish_restart"), &gPLConfig.fishing.restartDelayMs, 800, 5000);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::Separator();
    OverlaySnapshot::View snap{};
    OverlaySnapshot::Read(snap);
    if (snap.ready) {
        ImGui::Text(OBF("Trạng thái: %s"), OverlaySnapshot::FishingStateLabel(snap.fishingState));
        ImGui::Text(OBF("Đã câu (phiên): %d"), snap.fishCaught);
    } else {
        ImGui::TextUnformatted(OBF("Trạng thái: —"));
        ImGui::TextUnformatted(OBF("Đã câu (phiên): —"));
    }
    ImGui::TextUnformatted(OBF("Cần cầm cần câu, đứng vùng nước hợp lệ."));
    ImGui::PopID();
}

static void DrawTabSettings() {
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
}

}

void SetupMenuUi() {
    modui::AppUi ui{};
    ui.menu_size = ImVec2(720.f, 520.f);
    ui.fab_icon_path = OBF("/data/user/0/") + Tools::GetPackageName() + OBF("/files/fab.png");
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("fishing"), OBF("Câu Cá"), DrawTabFishing);
    ui.add_tab(OBF("settings"), OBF("Cài Đặt"), DrawTabSettings);
    modui::SetAppUi(ui);
}

}
