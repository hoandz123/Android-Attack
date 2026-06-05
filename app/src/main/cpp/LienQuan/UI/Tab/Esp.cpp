#include "Esp.h"
#include "../../Config/Config.h"
#include "../../Hook/EspHelper.h"
#include <Includes/obfuscate.h>
#include <imgui.h>

namespace lienquan {

namespace {

bool EspCheckbox(const char *label, bool *v) {
    const bool c = ImGui::Checkbox(label, v);
    if (c) SaveConfig();
    return c;
}

void EspColor(const char *label, float col[4]) {
    if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview)) SaveConfig();
}

void EspTip(const char *tip) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", tip);
}

} // namespace

void DrawEspPage() {
    if (ImGui::CollapsingHeader(OBF("ESP line"), ImGuiTreeNodeFlags_DefaultOpen)) {
        EspCheckbox(OBF("Bật ESP line##esp_on"), &gLQConfig.esp.enabled);
        EspTip(OBF("Vẽ line từ nhân vật tới mục tiêu"));
        ImGui::BeginDisabled(!gLQConfig.esp.enabled);
        EspCheckbox(OBF("Chỉ hiện địch##esp_enemy"), &gLQConfig.esp.enemiesOnly);
        EspTip(OBF("Ẩn đồng đội, chỉ vẽ line tới kẻ địch"));
        if (ImGui::SliderFloat(OBF("Độ dày line##esp_thick"), &gLQConfig.esp.lineThickness, 1.f, 6.f, "%.1f")) SaveConfig();
        EspTip(OBF("Độ dày đường line trên màn hình"));
        EspColor(OBF("Màu line##esp_line_col"), gLQConfig.esp.lineColor);
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader(OBF("Icon tướng"), ImGuiTreeNodeFlags_DefaultOpen)) {
        EspCheckbox(OBF("Hiện icon tướng##esp_icon"), &gLQConfig.esp.showHeroIcons);
        EspTip(OBF("Hiện icon tướng trên minimap"));
        ImGui::BeginDisabled(!gLQConfig.esp.showHeroIcons);
        if (ImGui::SliderFloat(OBF("Cỡ icon minimap##esp_mapsz"), &gLQConfig.esp.miniMapIconSize, 10.f, 60.f, "%.1f")) SaveConfig();
        EspTip(OBF("Kích thước icon tướng trên bản đồ nhỏ"));
        EspColor(OBF("Màu viền icon##esp_icon_border"), gLQConfig.esp.iconBorderColor);
        EspColor(OBF("Màu nền icon##esp_icon_shadow"), gLQConfig.esp.iconShadowColor);
        ImGui::EndDisabled();
    }
}

void RegisterEspOverlay() { EspHelper::Draw::Register(); }

}
