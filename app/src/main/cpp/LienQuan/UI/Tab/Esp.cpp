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

bool EspCombo(const char *label, int *v, const char *items) {
    const bool c = ImGui::Combo(label, v, items);
    if (c) SaveConfig();
    return c;
}

} // namespace

void DrawEspPage() {
    if (!ImGui::CollapsingHeader(OBF("ESP"), ImGuiTreeNodeFlags_DefaultOpen)) return;
    EspCheckbox(OBF("Bật ESP line"), &gLQConfig.esp.enabled);
    ImGui::BeginDisabled(!gLQConfig.esp.enabled);
    EspCheckbox(OBF("Chỉ địch"), &gLQConfig.esp.enemiesOnly);
    EspCheckbox(OBF("Chấm minimap"), &gLQConfig.esp.minimapDot);
    EspCheckbox(OBF("Debug pos"), &gLQConfig.esp.showDebug);
    EspCheckbox(OBF("Hiện icon tướng"), &gLQConfig.esp.showHeroIcons);
    ImGui::TextDisabled("%s", OBF("Icon: CHeroInfo.GetHeroName + ảnh nhúng HeroIcon (web)."));
    ImGui::TextDisabled("%s", OBF("ESP tick: LGameActorMgr.UpdateLogic (logic HeroActors)."));
    if (ImGui::SliderFloat(OBF("Độ dày line"), &gLQConfig.esp.lineThickness, 1.f, 6.f, "%.1f")) SaveConfig();
    ImGui::BeginDisabled(!gLQConfig.esp.minimapDot);
    if (ImGui::SliderFloat(OBF("Cỡ chấm map"), &gLQConfig.esp.minimapDotRadius, 2.f, 8.f, "%.1f")) SaveConfig();
    ImGui::EndDisabled();
    ImGui::TextDisabled("%s", OBF("Line + chấm vàng trên minimap (Mini/Big/Skill) — vị trí logic LActorRoot."));
    ImGui::EndDisabled();
}

void RegisterEspOverlay() { EspHelper::Draw::Register(); }

}
