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
    if (!ImGui::BeginTabBar(OBF("##esp_tabs"), ImGuiTabBarFlags_None)) return;

    if (ImGui::BeginTabItem(OBF("WorldPos"))) {
        ImGui::BeginChild(OBF("esp_world##scroll"), ImVec2(0, 0));
        if (ImGui::CollapsingHeader(OBF("ESP line"), ImGuiTreeNodeFlags_DefaultOpen)) {
            EspCheckbox(OBF("Bật ESP line##esp_on"), &gLQConfig.esp.enabled);
            EspTip(OBF("Vẽ line từ nhân vật tới mục tiêu"));
            ImGui::BeginDisabled(!gLQConfig.esp.enabled);
            if (ImGui::SliderFloat(OBF("Độ dày line##esp_thick"), &gLQConfig.esp.lineThickness, 1.f, 6.f, "%.1f")) SaveConfig();
            EspTip(OBF("Độ dày đường line trên màn hình"));
            EspColor(OBF("Màu line##esp_line_col"), gLQConfig.esp.lineColor);
            ImGui::EndDisabled();
        }
        if (ImGui::CollapsingHeader(OBF("Thông tin mục tiêu"), ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTabBar(OBF("##esp_info_tabs"), ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem(OBF("Chung"))) {
                    EspCheckbox(OBF("Hiện thông tin##esp_info"), &gLQConfig.esp.showInfo);
                    EspTip(OBF("Chỉ bật/tắt nhãn chữ; Máu, Hồi chiêu, Offscreen bật riêng"));
                    ImGui::BeginDisabled(!gLQConfig.esp.showInfo);
                    EspColor(OBF("Màu chữ##esp_info_col"), gLQConfig.esp.infoColor);
                    EspCheckbox(OBF("Khoảng cách##esp_dist"), &gLQConfig.esp.showDistance);
                    EspTip(OBF("Hiện khoảng cách tới mục tiêu trong nhãn"));
                    const char *layoutItems[] = {OBF("Dọc"), OBF("Ngang")};
                    if (ImGui::Combo(OBF("Bố cục info##esp_info_layout"), &gLQConfig.esp.infoLayout, layoutItems, 2)) SaveConfig();
                    EspTip(OBF("Xếp thanh máu dưới hoặc bên cạnh chữ"));
                    if (ImGui::SliderFloat(OBF("Lệch chữ X##esp_info_ox"), &gLQConfig.esp.infoOffsetX, -80.f, 80.f, "%.0f")) SaveConfig();
                    EspTip(OBF("Dịch nhãn thông tin theo trục ngang"));
                    if (ImGui::SliderFloat(OBF("Lệch chữ Y##esp_info_oy"), &gLQConfig.esp.infoOffsetY, -80.f, 80.f, "%.0f")) SaveConfig();
                    EspTip(OBF("Dịch nhãn thông tin theo trục dọc"));
                    ImGui::EndDisabled();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(OBF("Máu"))) {
                    EspCheckbox(OBF("Thanh máu##esp_hpbar"), &gLQConfig.esp.showHpBar);
                    EspTip(OBF("Vẽ thanh máu nhỏ gần mục tiêu, không cần bật nhãn chữ"));
                    ImGui::BeginDisabled(!gLQConfig.esp.showInfo);
                    EspCheckbox(OBF("Cảnh báo máu thấp##esp_lowhp"), &gLQConfig.esp.lowHpHighlight);
                    EspTip(OBF("Tô đỏ nhãn khi máu dưới 25%"));
                    ImGui::EndDisabled();
                    ImGui::BeginDisabled(!gLQConfig.esp.showHpBar);
                    if (ImGui::SliderFloat(OBF("Rộng thanh máu##esp_hp_w"), &gLQConfig.esp.hpBarWidth, 30.f, 200.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Cao thanh máu##esp_hp_h"), &gLQConfig.esp.hpBarHeight, 3.f, 20.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Lệch thanh máu X##esp_hp_ox"), &gLQConfig.esp.hpBarOffsetX, -80.f, 80.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Lệch thanh máu Y##esp_hp_oy"), &gLQConfig.esp.hpBarOffsetY, -40.f, 40.f, "%.0f")) SaveConfig();
                    ImGui::EndDisabled();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(OBF("Hồi chiêu"))) {
                    EspCheckbox(OBF("Hồi chiêu##esp_cd"), &gLQConfig.esp.showCooldowns);
                    EspTip(OBF("Hiện CD 4 chiêu: số giây khi đang hồi, chấm sáng khi sẵn sàng"));
                    ImGui::BeginDisabled(!gLQConfig.esp.showCooldowns);
                    if (ImGui::SliderFloat(OBF("Cỡ chấm CD##esp_cd_dot"), &gLQConfig.esp.cooldownDotSize, 3.f, 12.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Cỡ chữ CD##esp_cd_txt"), &gLQConfig.esp.cooldownTextSize, 8.f, 18.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Khoảng cách slot##esp_cd_sp"), &gLQConfig.esp.cooldownSpacing, 10.f, 28.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Lệch CD X##esp_cd_ox"), &gLQConfig.esp.cooldownOffsetX, -40.f, 40.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Lệch CD Y##esp_cd_oy"), &gLQConfig.esp.cooldownOffsetY, -20.f, 30.f, "%.0f")) SaveConfig();
                    ImGui::EndDisabled();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(OBF("Offscreen"))) {
                    EspCheckbox(OBF("Mũi tên offscreen##esp_arrow"), &gLQConfig.esp.offscreenArrow);
                    EspTip(OBF("Mũi tên ở mép màn hình khi mục tiêu ngoài tầm nhìn"));
                    ImGui::BeginDisabled(!gLQConfig.esp.offscreenArrow);
                    if (ImGui::SliderFloat(OBF("Cỡ mũi tên##esp_arrow_sz"), &gLQConfig.esp.arrowSize, 6.f, 30.f, "%.0f")) SaveConfig();
                    if (ImGui::SliderFloat(OBF("Lề mép màn hình##esp_arrow_mg"), &gLQConfig.esp.arrowMargin, 10.f, 120.f, "%.0f")) SaveConfig();
                    EspTip(OBF("Khoảng cách mũi tên tới mép màn hình"));
                    ImGui::EndDisabled();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(OBF("Mini map"))) {
        ImGui::BeginChild(OBF("esp_minimap##scroll"), ImVec2(0, 0));
        EspCheckbox(OBF("Hiện icon tướng##esp_icon"), &gLQConfig.esp.showHeroIcons);
        EspTip(OBF("Hiện icon tướng trên minimap"));
        ImGui::BeginDisabled(!gLQConfig.esp.showHeroIcons);
        if (ImGui::SliderFloat(OBF("Cỡ icon minimap##esp_mapsz"), &gLQConfig.esp.miniMapIconSize, 10.f, 60.f, "%.1f")) SaveConfig();
        EspTip(OBF("Kích thước icon tướng trên bản đồ nhỏ"));
        EspColor(OBF("Màu viền icon##esp_icon_border"), gLQConfig.esp.iconBorderColor);
        EspColor(OBF("Màu nền icon##esp_icon_shadow"), gLQConfig.esp.iconShadowColor);
        ImGui::EndDisabled();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void RegisterEspOverlay() { EspHelper::Draw::Register(); }

}
