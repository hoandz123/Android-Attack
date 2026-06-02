#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "OverlaySnapshot.h"
#include "../AutoFishing.h"
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

static void DrawFishingLiveStats(const OverlaySnapshot::View &snap) {
    ImGui::Text(OBF("Trạng thái: %s"), OverlaySnapshot::FishingStateLabel(snap.fishingState));
    ImGui::Text(OBF("Đã câu (phiên): %d"), snap.fishCaught);
    if (gPLConfig.fishing.showSessionStats) {
        unsigned int sec = snap.sessionSec;
        ImGui::Text(OBF("Thời gian: %um %us"), sec / 60, sec % 60);
        ImGui::Text(OBF("Hụt: %d | Trượt: %d"), snap.failCount, snap.missCount);
        ImGui::Text(OBF("C/B/A/S/SS: %d/%d/%d/%d/%d"), snap.catchGrade1, snap.catchGrade2, snap.catchGrade3, snap.catchGrade4, snap.catchGrade5);
    }
    if (gPLConfig.fishing.showEfficiency && snap.sessionSec >= 30) {
        ImGui::Text(OBF("Hiệu suất: ~%d con/giờ | thành công %d%%"), snap.catchesPerHour, snap.successRatePct);
    }
    if (gPLConfig.fishing.showFailHint && snap.lastFailType > 0) {
        ImGui::Text(OBF("Lỗi cast gần nhất: %s"), OverlaySnapshot::FailTypeLabel(snap.lastFailType));
    }
    if (snap.castFailStreak > 1 && gPLConfig.fishing.adaptiveCastBackoff) {
        ImGui::Text(OBF("Backoff cast: chuỗi lỗi %d"), snap.castFailStreak);
    }
    if (snap.fishingCountOver) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.35f, 0.35f, 1.f));
        ImGui::TextUnformatted(OBF("Đã hết lượt câu hôm nay"));
        ImGui::PopStyleColor();
    }
    if (gPLConfig.fishing.showBigFishHp && gPLConfig.fishing.handleBigFish && snap.bigFishHpMax > 0) {
        float frac = (float) snap.bigFishHp / (float) snap.bigFishHpMax;
        if (frac < 0.f) frac = 0.f;
        if (frac > 1.f) frac = 1.f;
        ImGui::ProgressBar(frac, ImVec2(-1, 0), OBF("HP cá lớn"));
    }
    if (snap.lastCatchItemId > 0) ImGui::Text(OBF("Cá vừa: ID %u (%s)"), snap.lastCatchItemId, OverlaySnapshot::GradeLabel(snap.lastCatchGrade));
    if (gPLConfig.fishing.showZoneInfo && snap.castingZoneId > 0) ImGui::Text(OBF("Vùng cast: %u | bắt: %u"), snap.castingZoneId, snap.catchZoneId);
    if (snap.baitUid != 0) ImGui::Text(OBF("Mồi UID: %lld"), (long long) snap.baitUid);
    if (snap.pausedByRare) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.85f, 0.2f, 1.f));
        ImGui::TextUnformatted(OBF("Tạm dừng: trúng cá hiếm"));
        ImGui::PopStyleColor();
        if (ImGui::Button(OBF("Tiếp tục câu##fish_resume"), ImVec2(-1, 0))) {
            AutoFishing::ClearRareAlert();
            SaveConfig();
        }
    }
    if (snap.rareAlert && !snap.pausedByRare) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.f, 0.5f, 1.f));
        ImGui::TextUnformatted(OBF("Vừa có cá hiếm — xem hộp thưởng"));
        ImGui::PopStyleColor();
    }
}

static void DrawTabFishing() {
    ImGui::PushID(OBF("fishing_tab"));
    UiCheckbox(OBF("Bật câu cá tự động"), &gPLConfig.fishing.enabled);
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Hộp thưởng & lọc"));
    UiCheckbox(OBF("Đóng hộp thưởng"), &gPLConfig.fishing.autoCloseReward);
    if (ImGui::Checkbox(OBF("Tự bán cá rác (rủi ro)"), &gPLConfig.fishing.autoSellTrash)) SaveConfig();
    if (gPLConfig.fishing.autoSellTrash) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
        ImGui::TextUnformatted(OBF("Cảnh báo: gọi bán qua dialog — có thể lệch server"));
        ImGui::PopStyleColor();
        ImGui::SliderInt(OBF("Bán tối đa grade##fish_sell_grade"), &gPLConfig.fishing.maxSellGrade, 1, 3);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    UiCheckbox(OBF("Dừng khi hết lượt câu"), &gPLConfig.fishing.stopWhenCountOver);
    if (ImGui::Checkbox(OBF("Dừng khi cá hiếm (S+)"), &gPLConfig.fishing.pauseOnRareCatch)) SaveConfig();
    if (gPLConfig.fishing.pauseOnRareCatch) {
        ImGui::SliderInt(OBF("Ngưỡng hiếm (grade)##fish_rare_grade"), &gPLConfig.fishing.minRareGrade, 3, 5);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Nhịp & hiển thị"));
    UiCheckbox(OBF("Hiện trạng thái"), &gPLConfig.fishing.showStatus);
    UiCheckbox(OBF("Thống kê phiên"), &gPLConfig.fishing.showSessionStats);
    UiCheckbox(OBF("Hiện vùng / mồi"), &gPLConfig.fishing.showZoneInfo);
    UiCheckbox(OBF("Gợi ý lỗi cast"), &gPLConfig.fishing.showFailHint);
    UiCheckbox(OBF("Hiệu suất phiên"), &gPLConfig.fishing.showEfficiency);
    UiCheckbox(OBF("Hướng phao (2D)"), &gPLConfig.fishing.showFloatMarker);
    UiCheckbox(OBF("Backoff cast thông minh"), &gPLConfig.fishing.adaptiveCastBackoff);
    ImGui::SliderInt(OBF("Nhịp tick (ms)##fish_tick"), &gPLConfig.fishing.tickIntervalMs, 250, 1200);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::SliderInt(OBF("Nhịp thao tác (ms)##fish_act"), &gPLConfig.fishing.actionIntervalMs, 300, 2000);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::SliderInt(OBF("Chờ cast lại (ms)##fish_restart"), &gPLConfig.fishing.restartDelayMs, 800, 5000);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::SliderInt(OBF("Trễ giật cắn (ms)##fish_hit_delay"), &gPLConfig.fishing.hitDelayMs, 0, 1500);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    ImGui::SliderInt(OBF("Trễ kéo (ms)##fish_lift_delay"), &gPLConfig.fishing.liftDelayMs, 0, 1500);
    if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    UiCheckbox(OBF("Bỏ qua khoe nhanh"), &gPLConfig.fishing.skipBoastDelay);
    if (gPLConfig.fishing.skipBoastDelay) {
        ImGui::SliderInt(OBF("Chờ khoe (ms)##fish_boast"), &gPLConfig.fishing.boastSkipMs, 200, 2000);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Cá lớn"));
    if (ImGui::Checkbox(OBF("Cá lớn / raid (rủi ro)"), &gPLConfig.fishing.handleBigFish)) SaveConfig();
    if (gPLConfig.fishing.handleBigFish) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
        ImGui::TextUnformatted(OBF("Cảnh báo: dễ lệch server, mặc định tắt"));
        ImGui::PopStyleColor();
        UiCheckbox(OBF("Thanh HP cá lớn"), &gPLConfig.fishing.showBigFishHp);
    }
    ImGui::Separator();
    if (ImGui::Button(OBF("Reset thống kê phiên##fish_reset_stats"), ImVec2(-1, 0))) {
        AutoFishing::ResetSessionStats();
    }
    OverlaySnapshot::View snap{};
    OverlaySnapshot::Read(snap);
    if (snap.ready) DrawFishingLiveStats(snap);
    else {
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
