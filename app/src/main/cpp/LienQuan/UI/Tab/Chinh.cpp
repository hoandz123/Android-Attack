#include "Chinh.h"
#include "../../Config/Config.h"
#include <Includes/obfuscate.h>
#include <imgui.h>

namespace lienquan {

namespace {

bool UiCheckbox(const char *label, bool *v) {
    bool changed = ImGui::Checkbox(label, v);
    if (changed) SaveConfig();
    return changed;
}

} // namespace

void DrawChinhPage() {
    if (ImGui::CollapsingHeader(OBF("Liên Quân"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(OBF("Tab Chính — cấu hình lưu vào lq_config.json"));
        UiCheckbox(OBF("Aim assist (demo)"), &gLQConfig.main.aimAssist);
        UiCheckbox(OBF("Map sáng"), &gLQConfig.main.mapHack);
    }
}

} // namespace lienquan
