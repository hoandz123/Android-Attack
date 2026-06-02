#include "MenuUi.h"
#include "Config/Config.h"
#include "Config/PLConfig.h"
#include "OverlaySnapshot.h"
#include "../AutoFishing.h"
#include "../FishingCatalog.h"
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

static void DrawRiskHint(const char *text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.45f, 0.2f, 1.f));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

static bool MatchSearch(const char *label, const char *search) {
    if (!search || !search[0]) return true;
    if (!label) return false;
    if (strstr(label, search)) return true;
    return false;
}

static bool FishPickerTypeVisible(int fishType, unsigned int mask) {
    if (fishType <= 0 || fishType >= 9) return false;
    if (fishType >= 4) return (mask & ((1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 8))) != 0;
    return (mask & (1u << (unsigned int) fishType)) != 0;
}

static bool DrawFishTypeFilter() {
    unsigned int &mask = gPLConfig.fishing.fishPickerTypeMask;
    bool showNormal = (mask & (1u << 1)) != 0;
    bool showKing = (mask & (1u << 2)) != 0;
    bool showJunk = (mask & (1u << 3)) != 0;
    bool showOther = (mask & ((1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 8))) != 0;
    bool changed = false;
    ImGui::TextUnformatted(OBF("Loại cá hiển thị:"));
    if (ImGui::Checkbox(OBF("Thường##fish_type_normal"), &showNormal)) changed = true;
    ImGui::SameLine();
    if (ImGui::Checkbox(OBF("Vua##fish_type_king"), &showKing)) changed = true;
    ImGui::SameLine();
    if (ImGui::Checkbox(OBF("Rác##fish_type_junk"), &showJunk)) changed = true;
    ImGui::SameLine();
    if (ImGui::Checkbox(OBF("Khác##fish_type_other"), &showOther)) changed = true;
    if (changed) {
        mask = 0;
        if (showNormal) mask |= (1u << 1);
        if (showKing) mask |= (1u << 2);
        if (showJunk) mask |= (1u << 3);
        if (showOther) mask |= ((1u << 4) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 8));
        if (mask == 0) mask = (1u << 1);
        SaveConfig();
    }
    return changed;
}

static const char *FindBaitLabel(const FishingCatalog::Snapshot &cat, int baitItemId) {
    for (int i = 0; i < cat.baitCount; i++) {
        if ((int) cat.baits[i].itemId == baitItemId) return cat.baits[i].label;
    }
    return nullptr;
}

static const char *FindZoneLabel(const FishingCatalog::Snapshot &cat, unsigned int zoneId) {
    for (int i = 0; i < cat.zoneCount; i++) {
        if (cat.zones[i].zoneId == zoneId) return cat.zones[i].label;
    }
    return nullptr;
}

static const char *FindGuideLabel(const FishingCatalog::Snapshot &cat, int guidePointId) {
    for (int i = 0; i < cat.guideCount; i++) {
        if (cat.guides[i].guidePointId == guidePointId) return cat.guides[i].label;
    }
    return nullptr;
}

static const char *FindFishLabel(const FishingCatalog::Snapshot &cat, int fishItemId) {
    for (int i = 0; i < cat.fishCount; i++) {
        if ((int) cat.fish[i].itemId == fishItemId) return cat.fish[i].label;
    }
    return nullptr;
}

static bool DrawBaitPicker(const char *id, int *baitItemId) {
    if (!baitItemId) return false;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    char preview[128];
    if (*baitItemId > 0) {
        const char *lbl = cat.ready ? FindBaitLabel(cat, *baitItemId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Mồi ID %d"), *baitItemId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Mồi##bait_combo"), preview)) {
        FishingCatalog::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.baitCount <= 0) ImGui::TextUnformatted(OBF("Không có mồi trong túi"));
        for (int i = 0; i < cat.baitCount; i++) {
            const auto &e = cat.baits[i];
            ImGui::PushID(i);
            bool selected = ((int) e.itemId == *baitItemId);
            if (ImGui::Selectable(e.label, selected)) {
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
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    char preview[128];
    if (*zoneId > 0) {
        const char *lbl = cat.ready ? FindZoneLabel(cat, *zoneId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Vùng %u"), *zoneId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(tự / chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Vùng câu##zone_combo"), preview)) {
        FishingCatalog::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.zoneCount <= 0) ImGui::TextUnformatted(OBF("Chưa có vùng (vào game?)"));
        if (ImGui::Selectable(OBF("(Không chọn / 0)"), *zoneId == 0)) {
            *zoneId = 0;
            changed = true;
        }
        for (int i = 0; i < cat.zoneCount; i++) {
            const auto &e = cat.zones[i];
            ImGui::PushID(i);
            bool selected = (e.zoneId == *zoneId);
            if (ImGui::Selectable(e.label, selected)) {
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

static bool DrawGuidePicker(const char *id, int *guidePointId) {
    if (!guidePointId) return false;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    char preview[128];
    if (*guidePointId > 0) {
        const char *lbl = cat.ready ? FindGuideLabel(cat, *guidePointId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Guide %d"), *guidePointId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Điểm guide##guide_combo"), preview)) {
        FishingCatalog::NotifyPickerOpen();
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.guideCount <= 0) ImGui::TextUnformatted(OBF("Chưa có guide (AutoCatchArea / map)"));
        bool wroteOtherHeader = false;
        for (int i = 0; i < cat.guideCount; i++) {
            const auto &e = cat.guides[i];
            if (i == 0 && e.onCurrentMap) ImGui::TextUnformatted(OBF("— Map hiện tại —"));
            if (!e.onCurrentMap && !wroteOtherHeader) {
                if (i > 0) ImGui::Separator();
                ImGui::TextUnformatted(OBF("— Khác —"));
                wroteOtherHeader = true;
            }
            char display[FishingCatalog::kLabelLen + 8];
            if (e.onCurrentMap) snprintf(display, sizeof(display), OBF("• %s"), e.label);
            else snprintf(display, sizeof(display), "%s", e.label);
            ImGui::PushID(i);
            bool selected = (e.guidePointId == *guidePointId);
            if (ImGui::Selectable(display, selected)) {
                *guidePointId = e.guidePointId;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::InputInt(OBF("Hoặc ID tay##guide_manual"), guidePointId, 0, 0);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::PopID();
    return changed;
}

static bool DrawFishPicker(const char *id, int *fishItemId) {
    if (!fishItemId) return false;
    static char s_fishSearch[64] = {};
    static int s_minGrade = 0;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    char preview[128];
    if (*fishItemId > 0) {
        const char *lbl = cat.ready ? FindFishLabel(cat, *fishItemId) : nullptr;
        if (lbl) snprintf(preview, sizeof(preview), "%s", lbl);
        else snprintf(preview, sizeof(preview), OBF("Cá ID %d"), *fishItemId);
    } else {
        snprintf(preview, sizeof(preview), "%s", OBF("(chưa chọn / tắt)"));
    }
    ImGui::PushID(id);
    if (ImGui::BeginCombo(OBF("Cá mục tiêu##fish_combo"), preview)) {
        FishingCatalog::NotifyPickerOpen();
        DrawFishTypeFilter();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint(OBF("##fish_search"), OBF("Tìm tên/ID…"), s_fishSearch, sizeof(s_fishSearch));
        ImGui::SliderInt(OBF("Grade tối thiểu##fish_grade_min"), &s_minGrade, 0, 5);
        if (!cat.ready) ImGui::TextUnformatted(OBF("Đang tải danh sách…"));
        else if (cat.fishCount <= 0) ImGui::TextUnformatted(OBF("Chưa có cá (Fishlist)"));
        if (ImGui::Selectable(OBF("(Tắt / 0)"), *fishItemId == 0)) {
            *fishItemId = 0;
            changed = true;
        }
        ImGui::BeginChild(OBF("fish_list##child"), ImVec2(0, 180), true);
        unsigned int typeMask = gPLConfig.fishing.fishPickerTypeMask;
        for (int i = 0; i < cat.fishCount; i++) {
            const auto &e = cat.fish[i];
            if (!FishPickerTypeVisible(e.fishType, typeMask)) continue;
            if (s_minGrade > 0 && e.grade < s_minGrade) continue;
            if (!MatchSearch(e.label, s_fishSearch)) {
                char idBuf[16];
                snprintf(idBuf, sizeof(idBuf), "%u", e.itemId);
                if (!MatchSearch(idBuf, s_fishSearch)) continue;
            }
            ImGui::PushID(i);
            bool selected = ((int) e.itemId == *fishItemId);
            if (ImGui::Selectable(e.label, selected)) {
                *fishItemId = (int) e.itemId;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::EndCombo();
    }
    ImGui::InputInt(OBF("Hoặc ID tay##fish_manual"), fishItemId, 0, 0);
    if (ImGui::IsItemDeactivatedAfterEdit()) changed = true;
    ImGui::PopID();
    return changed;
}

static bool DrawZoneBaitPrefEditor() {
    static unsigned int s_pickZone = 0;
    static int s_pickBait = 0;
    FishingCatalog::Snapshot cat{};
    FishingCatalog::Read(cat);
    bool changed = false;
    ImGui::TextUnformatted(OBF("Ưu tiên mồi theo vùng:"));
    if (DrawZonePicker(OBF("pref_zone"), &s_pickZone)) changed = true;
    if (DrawBaitPicker(OBF("pref_bait"), &s_pickBait)) changed = true;
    if (ImGui::Button(OBF("Thêm/cập nhật ưu tiên##zone_bait_add"), ImVec2(-1, 0))) {
        if (s_pickZone > 0 && s_pickBait > 0) {
            bool found = false;
            for (auto &p : gPLConfig.fishing.baitZonePrefs) {
                if (p.first == s_pickZone) {
                    p.second = (unsigned int) s_pickBait;
                    found = true;
                    break;
                }
            }
            if (!found) gPLConfig.fishing.baitZonePrefs.emplace_back(s_pickZone, (unsigned int) s_pickBait);
            changed = true;
        }
    }
    for (size_t i = 0; i < gPLConfig.fishing.baitZonePrefs.size(); i++) {
        const auto &p = gPLConfig.fishing.baitZonePrefs[i];
        const char *zLbl = FindZoneLabel(cat, p.first);
        const char *bLbl = FindBaitLabel(cat, (int) p.second);
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
    return changed;
}

static void DrawFishingLiveStats(const OverlaySnapshot::View &snap) {
    ImGui::Text(OBF("Trạng thái: %s"), OverlaySnapshot::FishingStateLabel(snap.fishingState));
    if (snap.currentFishLevel > 0) {
        ImGui::Text(OBF("Cá hiện tại: bóng %s | lv %u"), OverlaySnapshot::ShadowLabel(snap.currentShadowIndex), snap.currentFishLevel);
    }
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
    if (snap.statusHint) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.35f, 1.f));
        ImGui::Text(OBF("Gợi ý: %s"), snap.statusHint);
        ImGui::PopStyleColor();
    }
    if (snap.pausedByRare) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.85f, 0.2f, 1.f));
        ImGui::TextUnformatted(gPLConfig.fishing.targetFishItemId > 0 ? OBF("Tạm dừng: đủ cá mục tiêu / hiếm") : OBF("Tạm dừng: trúng cá hiếm"));
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

static void DrawTabFish() {
    ImGui::PushID(OBF("tab_fish"));
    UiCheckbox(OBF("Bật câu cá tự động"), &gPLConfig.fishing.enabled);
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Nhịp cast / giật / kéo"));
    UiCheckbox(OBF("Nhịp chống captcha"), &gPLConfig.fishing.adaptivePacing);
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
    UiCheckbox(OBF("Giật chuẩn (timing phao)"), &gPLConfig.fishing.autoPerfectTug);
    if (gPLConfig.fishing.autoPerfectTug) {
        ImGui::SliderInt(OBF("Nhịp giật (ms)##fish_perfect"), &gPLConfig.fishing.perfectLiftIntervalMs, 120, 600);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    if (ImGui::Checkbox(OBF("Cắn nhanh (rủi ro)"), &gPLConfig.fishing.fastBite)) SaveConfig();
    if (gPLConfig.fishing.fastBite) DrawRiskHint(OBF("Cảnh báo: ép FishingBite — có thể lệch server"));
    UiCheckbox(OBF("Bỏ qua khoe nhanh"), &gPLConfig.fishing.skipBoastDelay);
    if (gPLConfig.fishing.skipBoastDelay) {
        ImGui::SliderInt(OBF("Chờ khoe (ms)##fish_boast"), &gPLConfig.fishing.boastSkipMs, 200, 2000);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    UiCheckbox(OBF("Dừng khi hết lượt câu"), &gPLConfig.fishing.stopWhenCountOver);
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Hiển thị"));
    UiCheckbox(OBF("Hiện trạng thái"), &gPLConfig.fishing.showStatus);
    UiCheckbox(OBF("Thống kê phiên"), &gPLConfig.fishing.showSessionStats);
    UiCheckbox(OBF("Hiện vùng / mồi"), &gPLConfig.fishing.showZoneInfo);
    UiCheckbox(OBF("Gợi ý lỗi cast"), &gPLConfig.fishing.showFailHint);
    UiCheckbox(OBF("Hiệu suất phiên"), &gPLConfig.fishing.showEfficiency);
    UiCheckbox(OBF("Hướng phao (2D)"), &gPLConfig.fishing.showFloatMarker);
    ImGui::Separator();
    if (ImGui::Button(OBF("Reset thống kê phiên##fish_reset_stats"), ImVec2(-1, 0))) AutoFishing::ResetSessionStats();
    OverlaySnapshot::View snap{};
    OverlaySnapshot::Read(snap);
    if (snap.ready) DrawFishingLiveStats(snap);
    else ImGui::TextUnformatted(OBF("Trạng thái: —"));
    ImGui::TextUnformatted(OBF("Cần cầm cần câu, đứng vùng nước hợp lệ."));
    ImGui::PopID();
}

static void DrawTabFilter() {
    ImGui::PushID(OBF("tab_filter"));
    ImGui::TextUnformatted(OBF("Lọc theo bóng"));
    UiCheckbox(OBF("Bật lọc bóng"), &gPLConfig.fishing.filterByShadow);
    if (gPLConfig.fishing.filterByShadow) {
        static const char *kShadowLabels[7] = {OBF("S##sh1"), OBF("M##sh2"), OBF("L##sh3"), OBF("XL##sh4"), OBF("XXL##sh5"), OBF("XXXL##sh6"), OBF("4XL##sh7")};
        ImGui::TextUnformatted(OBF("Giữ bóng:"));
        for (int i = 0; i < 7; i++) {
            ImGui::PushID(i);
            if (ImGui::Checkbox(kShadowLabels[i], &gPLConfig.fishing.keepShadow[i])) SaveConfig();
            if (i < 6) ImGui::SameLine();
            ImGui::PopID();
        }
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Lọc theo level"));
    UiCheckbox(OBF("Bật lọc level"), &gPLConfig.fishing.filterByLevel);
    if (gPLConfig.fishing.filterByLevel) {
        ImGui::InputInt(OBF("Level tối thiểu (0=tắt)##fish_lv_min"), &gPLConfig.fishing.levelMin, 0, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::InputInt(OBF("Level tối đa (0=tắt)##fish_lv_max"), &gPLConfig.fishing.levelMax, 0, 0);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::TextUnformatted(OBF("Hoặc keepLevelIds trong config.json"));
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Hiếm & mục tiêu"));
    if (ImGui::Checkbox(OBF("Dừng khi cá hiếm (S+)"), &gPLConfig.fishing.pauseOnRareCatch)) SaveConfig();
    if (gPLConfig.fishing.pauseOnRareCatch) {
        ImGui::SliderInt(OBF("Ngưỡng hiếm (grade)##fish_rare_grade"), &gPLConfig.fishing.minRareGrade, 3, 5);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    if (DrawFishPicker(OBF("target_fish"), &gPLConfig.fishing.targetFishItemId)) SaveConfig();
    if (gPLConfig.fishing.targetFishItemId > 0) ImGui::TextUnformatted(OBF("Dừng cast khi bắt đúng cá mục tiêu — bấm Tiếp tục"));
    OverlaySnapshot::View snap{};
    OverlaySnapshot::Read(snap);
    if (snap.ready && snap.currentFishLevel > 0) {
        ImGui::Separator();
        ImGui::Text(OBF("Đang thấy: bóng %s | lv %u"), OverlaySnapshot::ShadowLabel(snap.currentShadowIndex), snap.currentFishLevel);
    }
    ImGui::PopID();
}

static void DrawTabBag() {
    ImGui::PushID(OBF("tab_bag"));
    UiCheckbox(OBF("Đóng hộp thưởng"), &gPLConfig.fishing.autoCloseReward);
    if (ImGui::Checkbox(OBF("Tự bán cá rác (rủi ro)"), &gPLConfig.fishing.autoSellTrash)) SaveConfig();
    if (gPLConfig.fishing.autoSellTrash) {
        DrawRiskHint(OBF("Cảnh báo: gọi bán qua dialog — có thể lệch server"));
        ImGui::SliderInt(OBF("Bán tối đa grade##fish_sell_grade"), &gPLConfig.fishing.maxSellGrade, 1, 3);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    if (ImGui::Checkbox(OBF("Bán/giữ theo túi (rủi ro)"), &gPLConfig.fishing.smartKeepSell)) SaveConfig();
    if (gPLConfig.fishing.smartKeepSell) {
        DrawRiskHint(OBF("Cảnh báo: giá trị + codex + áp lực túi"));
        ImGui::SliderInt(OBF("Giữ grade ≥##fish_keep_grade"), &gPLConfig.fishing.smartKeepMinGrade, 3, 5);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::SliderInt(OBF("Giữ nếu sở hữu <##fish_keep_cnt"), &gPLConfig.fishing.smartKeepMaxOwned, 1, 20);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::SliderInt(OBF("Bán nếu giá <##fish_min_sell"), &gPLConfig.fishing.minSellValue, 0, 5000);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        UiCheckbox(OBF("Giữ cá thiếu codex"), &gPLConfig.fishing.keepCodexFish);
    }
    ImGui::PopID();
}

static void DrawTabAdvanced() {
    ImGui::PushID(OBF("tab_adv"));
    ImGui::TextUnformatted(OBF("Cá lớn & raid"));
    if (ImGui::Checkbox(OBF("Cá lớn / raid (rủi ro)"), &gPLConfig.fishing.handleBigFish)) SaveConfig();
    if (gPLConfig.fishing.handleBigFish) {
        DrawRiskHint(OBF("Cảnh báo: dễ lệch server, mặc định tắt"));
        UiCheckbox(OBF("Thanh HP cá lớn"), &gPLConfig.fishing.showBigFishHp);
    }
    if (ImGui::Checkbox(OBF("Tự vào raid (rủi ro cao)"), &gPLConfig.fishing.autoRaidEnter)) SaveConfig();
    if (gPLConfig.fishing.autoRaidEnter) DrawRiskHint(OBF("Cảnh báo: SendToFishingRaidEnter — mặc định tắt"));
    UiCheckbox(OBF("Giật stun theo server"), &gPLConfig.fishing.stunOrchestrator);
    if (gPLConfig.fishing.stunOrchestrator) {
        ImGui::SliderInt(OBF("Nhịp stun (ms)##fish_stun_iv"), &gPLConfig.fishing.stunHitIntervalMs, 180, 800);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        ImGui::SliderInt(OBF("Tối đa stun/phase##fish_stun_cap"), &gPLConfig.fishing.maxStunHitsPerPhase, 1, 16);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Mồi & zone"));
    if (ImGui::Checkbox(OBF("Tự gắn mồi UID (rủi ro)"), &gPLConfig.fishing.autoEquipBait)) SaveConfig();
    if (gPLConfig.fishing.autoEquipBait) {
        DrawRiskHint(OBF("Cảnh báo: set_FishingBaitUID trước cast"));
        if (DrawBaitPicker(OBF("main_bait"), &gPLConfig.fishing.baitItemId)) SaveConfig();
        UiCheckbox(OBF("Mồi theo zone (config JSON)"), &gPLConfig.fishing.smartBaitByZone);
        UiCheckbox(OBF("Mồi auto EffectId/ActionId"), &gPLConfig.fishing.smartBaitAutoEffect);
        if (gPLConfig.fishing.smartBaitByZone) {
            if (DrawZoneBaitPrefEditor()) SaveConfig();
        }
    }
    ImGui::Separator();
    ImGui::TextUnformatted(OBF("Định tuyến & AFK phụ"));
    if (ImGui::Checkbox(OBF("Guide tới điểm câu (rủi ro)"), &gPLConfig.fishing.guideRouting)) SaveConfig();
    if (gPLConfig.fishing.guideRouting) {
        DrawRiskHint(OBF("Chỉ mũi tên + CheckFishingPoint — không teleport"));
        if (DrawGuidePicker(OBF("guide_pt"), &gPLConfig.fishing.guidePointId)) SaveConfig();
        ImGui::SliderInt(OBF("Ngưỡng lỗi cast liên tiếp##fish_guide_streak"), &gPLConfig.fishing.guideFailStreak, 2, 8);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
        if (DrawZonePicker(OBF("guide_zone"), &gPLConfig.fishing.guideTargetZoneId)) SaveConfig();
    }
    if (ImGui::Checkbox(OBF("Poll lưới AFK (CheckAutoCatch)"), &gPLConfig.fishing.autoCatchNetCheck)) SaveConfig();
    if (gPLConfig.fishing.autoCatchNetCheck) {
        DrawRiskHint(OBF("Chỉ CheckAutoCatchFishingNet — chưa AddAutoCatch"));
        ImGui::SliderInt(OBF("Nhịp poll (ms)##fish_autocatch_iv"), &gPLConfig.fishing.autoCatchIntervalMs, 5000, 30000);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    if (ImGui::Checkbox(OBF("Nhận thưởng NV ngày (rủi ro)"), &gPLConfig.fishing.autoDailyMissionReward)) SaveConfig();
    if (gPLConfig.fishing.autoDailyMissionReward) {
        DrawRiskHint(OBF("HasDailyMissionReward + SendToMissionReward"));
        ImGui::SliderInt(OBF("Nhịp claim (ms)##fish_mission_iv"), &gPLConfig.fishing.missionClaimIntervalMs, 2000, 5000);
        if (ImGui::IsItemDeactivatedAfterEdit()) SaveConfig();
    }
    ImGui::PopID();
}

static void DrawTabSettings() {
    ImGui::PushID(OBF("tab_settings"));
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
    ImGui::PopID();
}

static void DrawSubTabScroll(const char *id, void (*draw)()) {
    ImGui::PushID(id);
    ImGui::BeginChild(OBF("##sub_scroll"), ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    draw();
    ImGui::EndChild();
    ImGui::PopID();
}

static void DrawFishingPage() {
    ImGui::PushID(OBF("page_fishing"));
    if (ImGui::BeginTabBar(OBF("##pl_fishing_tabs"), ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem(OBF("Câu"))) {
            DrawSubTabScroll(OBF("sub_fish"), DrawTabFish);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Lọc"))) {
            DrawSubTabScroll(OBF("sub_filter"), DrawTabFilter);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Cá & Túi"))) {
            DrawSubTabScroll(OBF("sub_bag"), DrawTabBag);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Nâng cao"))) {
            DrawSubTabScroll(OBF("sub_adv"), DrawTabAdvanced);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(OBF("Cài đặt"))) {
            DrawSubTabScroll(OBF("sub_settings"), DrawTabSettings);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
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
