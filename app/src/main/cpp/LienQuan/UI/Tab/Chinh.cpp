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
        UiCheckbox(OBF("Map sáng"), &gLQConfig.main.mapHack);
        UiCheckbox(OBF("Block tải tài nguyên"), &gLQConfig.main.blockDlcDownload);
        UiCheckbox(OBF("Unlock skin"), &gLQConfig.main.unlockSkin);
        UiCheckbox(OBF("Unlock nút (PersonalButton)"), &gLQConfig.main.unlockButton);
        if (gLQConfig.main.unlockButton) {
            if (ImGui::InputInt(OBF("Hero ID"), &gLQConfig.main.buttonHeroId)) SaveConfig();
            if (ImGui::InputInt(OBF("Skin ID"), &gLQConfig.main.buttonSkinId)) SaveConfig();
        }
    }
}

} // namespace lienquan
