#include <imgui.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>

#define LOG_TAG OBF("AttackPlugin")
#include <Includes/Logger.h>

namespace playtogether {

const char* kPackage = OBF("com.vng.playtogether");

static void DrawMainTab() {
    if (ImGui::CollapsingHeader(OBF("Play Together"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(OBF("Menu Play Together — thêm tab/feature tại đây."));
        static bool speedDemo = false;
        ImGui::Checkbox(OBF("Speed hack (demo)"), &speedDemo);
        static bool flyDemo = false;
        ImGui::Checkbox(OBF("Fly (demo)"), &flyDemo);
    }
}

static void InitHooks() {
    LOGI(OBF("[PlayTogether] InitHooks"));
    // HOOK_LIB(OBF("libil2cpp.so"), OBF("0x0"), hook_Foo, old_Foo);
}

void Activate() {
    InitHooks();
    modui::AppUi ui{};
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("main"), OBF("Chính"), DrawMainTab);
    modui::SetAppUi(ui);
}

} // namespace playtogether
