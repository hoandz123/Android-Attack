#include "UI/Menu.h"
#include "UI/TouchInput.h"
#include "UI/FontRoboto.h"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include <chrono>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cstring>

namespace overlayMenu {

namespace {

struct MenuState {
    bool menuOpen = true;
    int activeTab = 0;
    float accent[4] = {0.902f, 0.098f, 0.098f, 1.0f};
    float menuScale = 1.0f;
    float windowAlpha = 0.97f;
    bool showWatermark = true;
    bool showFpsOverlay = true;
    int menuKey = 0;
    char spoofName[64] = "Player";
    char profileName[32] = "default";
    int configSlot = 0;
    char configStatus[128] = "Ready";
    bool aimEnabled = false;
    int aimKey = 0;
    float aimFov = 90.0f;
    float aimSmooth = 4.0f;
    int aimBone = 0;
    bool aimVisible = true;
    bool aimPrediction = false;
    bool aimRcs = false;
    int aimMode = 0;
    bool espMaster = false;
    int espBox = 0;
    float espBoxColor[4] = {0.2f, 0.9f, 0.4f, 1.0f};
    bool espSkeleton = false;
    float espSkelColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    bool espName = true;
    bool espDistance = true;
    bool espHealth = true;
    int espTracer = 0;
    float espTracerColor[4] = {0.9f, 0.2f, 0.2f, 1.0f};
    bool espFill = false;
    float espFillAlpha = 0.25f;
    bool espChams = false;
    bool espOffscreen = false;
    float espMaxDist = 500.0f;
    float playerSpeed = 1.0f;
    bool playerJump = false;
    bool playerNoRecoil = false;
    bool playerGod = false;
    bool playerFly = false;
    bool playerStamina = false;
    float playerHealth = 100.0f;
    float playerArmor = 0.0f;
    float worldTime = 12.0f;
    float worldGravity = 9.8f;
    bool worldFog = true;
    float worldFov = 90.0f;
    bool worldNoFog = false;
    float worldBrightness = 1.0f;
    bool wpnNoSpread = false;
    bool wpnFastReload = false;
    float wpnFireRate = 1.0f;
    bool wpnMagic = false;
    bool wpnInfAmmo = false;
    bool miscCrosshair = false;
    float miscCrossColor[4] = {0.902f, 0.098f, 0.098f, 1.0f};
    bool miscFps = true;
    bool miscWatermark = true;
    bool miscBypass = false;
    bool miscAntiAfk = false;
};

MenuState g_state;
bool g_themeReady = false;
bool g_accentDirty = true;
int g_dispW = 0;
int g_dispH = 0;
float g_androidScale = 1.0f;

constexpr float kMenuBaseW = 760.0f;
constexpr float kMenuBaseH = 520.0f;

void fitMenuSizeToScreen(float *winW, float *winH, float maxW, float maxH) {
    if (*winW <= maxW && *winH <= maxH) return;
    float scaleW = maxW / *winW;
    float scaleH = maxH / *winH;
    float fit = scaleW < scaleH ? scaleW : scaleH;
    *winW *= fit;
    *winH *= fit;
}

float computeAndroidScale(int w, int h) {
    if (w <= 0 || h <= 0) return 1.5f;
    float sw = ((float)w * 0.92f) / kMenuBaseW;
    float sh = ((float)h * 0.88f) / kMenuBaseH;
    float s = sw < sh ? sw : sh;
    if (s < 0.85f) s = 0.85f;
    if (s > 2.8f) s = 2.8f;
    return s;
}

void applyBaseStyleValues(ImGuiStyle &style) {
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowPadding = ImVec2(14.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(12.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 5.0f);
    style.ScrollbarSize = 10.0f;
    style.WindowBorderSize = 2.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
}

void applyQualityStyle(ImGuiStyle &style) {
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill = true;
    style.CurveTessellationTol = 0.25f;
    style.CircleTessellationMaxError = 0.05f;
    style.Alpha = 1.0f;
}

void applyCombinedScale(float scale) {
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::StyleColorsLight();
    applyBaseStyleValues(style);
    style.ScaleAllSizes(scale);
    applyQualityStyle(style);
    g_accentDirty = true;
}

float combinedUiScale() {
    return g_androidScale * g_state.menuScale;
}

void updateDisplayScale(int w, int h) {
    bool sizeChanged = (w != g_dispW || h != g_dispH);
    if (!sizeChanged && g_themeReady) return;
    g_dispW = w;
    g_dispH = h;
    g_androidScale = computeAndroidScale(w, h);
    applyCombinedScale(combinedUiScale());
    g_themeReady = true;
}

void tooltip(const char *text) {
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s", text);
}

void applyAccentColors() {
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec4 accent = ImVec4(g_state.accent[0], g_state.accent[1], g_state.accent[2], g_state.accent[3]);
    ImVec4 accentDim = ImVec4(accent.x * 0.72f, accent.y * 0.72f, accent.z * 0.72f, accent.w);
    ImVec4 ink = ImVec4(0.067f, 0.067f, 0.067f, 1.0f);
    ImVec4 paper = ImVec4(0.957f, 0.957f, 0.941f, g_state.windowAlpha);
    ImVec4 paperDim = ImVec4(0.918f, 0.910f, 0.890f, g_state.windowAlpha);
    ImVec4 rule = ImVec4(0.067f, 0.067f, 0.067f, 0.18f);
    style.Colors[ImGuiCol_WindowBg] = paper;
    style.Colors[ImGuiCol_ChildBg] = paperDim;
    style.Colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, g_state.windowAlpha);
    style.Colors[ImGuiCol_Border] = ink;
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.98f, 0.98f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.96f, 0.96f, 0.94f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = paperDim;
    style.Colors[ImGuiCol_TitleBgActive] = paper;
    style.Colors[ImGuiCol_TitleBgCollapsed] = paperDim;
    style.Colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.12f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.22f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.32f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(accent.x, accent.y, accent.z, 0.14f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(accent.x, accent.y, accent.z, 0.28f);
    style.Colors[ImGuiCol_CheckMark] = accent;
    style.Colors[ImGuiCol_SliderGrab] = accentDim;
    style.Colors[ImGuiCol_SliderGrabActive] = accent;
    style.Colors[ImGuiCol_Tab] = paperDim;
    style.Colors[ImGuiCol_TabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.18f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(accent.x, accent.y, accent.z, 0.24f);
    style.Colors[ImGuiCol_Separator] = accent;
    style.Colors[ImGuiCol_ScrollbarBg] = paperDim;
    style.Colors[ImGuiCol_ScrollbarGrab] = rule;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = accentDim;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = accent;
    style.Colors[ImGuiCol_Text] = ink;
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.42f, 0.42f, 1.0f);
    g_accentDirty = false;
}

void drawSwissHeader() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.067f, 0.067f, 0.067f, 1.0f));
    ImGui::SetWindowFontScale(1.12f);
    ImGui::Text("ANDROID-ATTACK");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::TextColored(ImVec4(g_state.accent[0], g_state.accent[1], g_state.accent[2], 1.0f), "MOD SYSTEM / INDUSTRIAL PRINT");
    ImGui::TextDisabled("SPEC REV 1.0  DEMO BUILD");
    ImGui::Separator();
}

void drawFovCircle(int displayWidth, int displayHeight) {
    if (!g_state.aimEnabled) return;
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    float cx = (float)displayWidth * 0.5f;
    float cy = (float)displayHeight * 0.5f;
    float radius = g_state.aimFov * 2.2f;
    ImU32 col = IM_COL32((int)(g_state.accent[0] * 255.0f), (int)(g_state.accent[1] * 255.0f), (int)(g_state.accent[2] * 255.0f), 180);
    dl->AddCircle(ImVec2(cx, cy), radius, col, 128, 2.5f);
    dl->AddLine(ImVec2(cx - 10.0f, cy), ImVec2(cx + 10.0f, cy), col, 2.0f);
    dl->AddLine(ImVec2(cx, cy - 10.0f), ImVec2(cx, cy + 10.0f), col, 2.0f);
}

void drawWatermark() {
    if (!g_state.showWatermark && !g_state.showFpsOverlay) return;
    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGuiWindowFlags wf = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("##wm", nullptr, wf)) {
        if (g_state.showWatermark) ImGui::TextColored(ImVec4(g_state.accent[0], g_state.accent[1], g_state.accent[2], 1.0f), "ANDROID-ATTACK");
        if (g_state.showFpsOverlay) ImGui::Text("FPS %.1f", io.Framerate);
        std::time_t now = std::time(nullptr);
        std::tm *lt = std::localtime(&now);
        if (lt) ImGui::TextDisabled("%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
    }
    ImGui::End();
}

void drawMiscCrosshair(int displayWidth, int displayHeight) {
    if (!g_state.miscCrosshair) return;
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    float cx = (float)displayWidth * 0.5f;
    float cy = (float)displayHeight * 0.5f;
    ImU32 col = IM_COL32((int)(g_state.miscCrossColor[0] * 255.0f), (int)(g_state.miscCrossColor[1] * 255.0f), (int)(g_state.miscCrossColor[2] * 255.0f), (int)(g_state.miscCrossColor[3] * 255.0f));
    dl->AddLine(ImVec2(cx - 16.0f, cy), ImVec2(cx - 5.0f, cy), col, 2.5f);
    dl->AddLine(ImVec2(cx + 5.0f, cy), ImVec2(cx + 16.0f, cy), col, 2.5f);
    dl->AddLine(ImVec2(cx, cy - 16.0f), ImVec2(cx, cy - 5.0f), col, 2.5f);
    dl->AddLine(ImVec2(cx, cy + 5.0f), ImVec2(cx, cy + 16.0f), col, 2.5f);
}

bool drawMenuToggleButton() {
    float s = combinedUiScale();
    ImGui::SetNextWindowPos(ImVec2(16.0f * s, 80.0f * s), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(96.0f * s, 48.0f * s), ImGuiCond_Always);
    ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;
    bool clicked = false;
    if (ImGui::Begin("##menu_toggle", nullptr, wf)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(g_state.accent[0], g_state.accent[1], g_state.accent[2], 0.85f));
        if (ImGui::Button("MENU", ImVec2(-1.0f, -1.0f))) { g_state.menuOpen = true; clicked = true; }
        ImGui::PopStyleColor();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        touchInput::setCaptureRect(pos.x, pos.y, size.x, size.y);
    }
    ImGui::End();
    return clicked;
}

void drawTabAimbot() {
    if (ImGui::CollapsingHeader("Aimbot Core", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Aimbot", &g_state.aimEnabled);
        tooltip("Master toggle for aim assistance (demo)");
        const char *keys[] = {"None", "LMB", "RMB", "Shift", "Alt"};
        ImGui::Combo("Aim Key", &g_state.aimKey, keys, IM_ARRAYSIZE(keys));
        ImGui::SliderFloat("FOV", &g_state.aimFov, 10.0f, 360.0f, "%.0f");
        ImGui::SliderFloat("Smoothing", &g_state.aimSmooth, 0.0f, 20.0f, "%.1f");
        const char *bones[] = {"Head", "Neck", "Chest", "Nearest"};
        ImGui::Combo("Target Bone", &g_state.aimBone, bones, IM_ARRAYSIZE(bones));
        ImGui::Checkbox("Visible Check", &g_state.aimVisible);
        ImGui::Checkbox("Prediction", &g_state.aimPrediction);
        ImGui::Checkbox("RCS", &g_state.aimRcs);
        const char *modes[] = {"Legit", "Rage", "Silent", "Trigger"};
        ImGui::Combo("Aim Mode", &g_state.aimMode, modes, IM_ARRAYSIZE(modes));
    }
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Advanced")) {
        static float hitchance = 75.0f;
        ImGui::SliderFloat("Hit Chance", &hitchance, 0.0f, 100.0f, "%.0f%%");
        static bool teamCheck = true;
        ImGui::Checkbox("Team Check", &teamCheck);
        static int maxTargets = 1;
        ImGui::SliderInt("Max Targets", &maxTargets, 1, 5);
    }
}

void drawTabVisuals() {
    ImGui::Checkbox("ESP Master", &g_state.espMaster);
    ImGui::SameLine();
    ImGui::ProgressBar(g_state.espMaster ? 1.0f : 0.0f, ImVec2(120.0f, 0.0f), g_state.espMaster ? "ON" : "OFF");
    if (ImGui::CollapsingHeader("Players", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char *boxes[] = {"2D Box", "3D Box", "Corner"};
        ImGui::Combo("Box Type", &g_state.espBox, boxes, IM_ARRAYSIZE(boxes));
        ImGui::ColorEdit4("Box Color", g_state.espBoxColor, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Skeleton", &g_state.espSkeleton);
        ImGui::ColorEdit4("Skeleton Color", g_state.espSkelColor, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Name", &g_state.espName);
        ImGui::Checkbox("Distance", &g_state.espDistance);
        ImGui::Checkbox("Health Bar", &g_state.espHealth);
    }
    if (ImGui::CollapsingHeader("Lines & Fill")) {
        const char *tracers[] = {"Bottom", "Center", "Top"};
        ImGui::Combo("Tracer From", &g_state.espTracer, tracers, IM_ARRAYSIZE(tracers));
        ImGui::ColorEdit4("Tracer Color", g_state.espTracerColor, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Fill Box", &g_state.espFill);
        ImGui::SliderFloat("Fill Alpha", &g_state.espFillAlpha, 0.0f, 1.0f);
        ImGui::Checkbox("Chams", &g_state.espChams);
        ImGui::Checkbox("Off-screen Arrow", &g_state.espOffscreen);
    }
    ImGui::SliderFloat("Max Distance", &g_state.espMaxDist, 50.0f, 2000.0f, "%.0f m");
}

void drawTabPlayer() {
    if (ImGui::BeginTable("##player_tbl", 2, ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::SliderFloat("Speed", &g_state.playerSpeed, 0.5f, 5.0f, "x%.2f");
        ImGui::Checkbox("Super Jump", &g_state.playerJump);
        ImGui::Checkbox("No Recoil", &g_state.playerNoRecoil);
        ImGui::Checkbox("God Mode", &g_state.playerGod);
        ImGui::TableSetColumnIndex(1);
        ImGui::Checkbox("Fly", &g_state.playerFly);
        ImGui::Checkbox("Infinite Stamina", &g_state.playerStamina);
        ImGui::SliderFloat("Health", &g_state.playerHealth, 0.0f, 200.0f);
        ImGui::SliderFloat("Armor", &g_state.playerArmor, 0.0f, 200.0f);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::Button("Teleport Spawn")) { std::strncpy(g_state.configStatus, "Teleport: Spawn (demo)", sizeof(g_state.configStatus)); }
    ImGui::SameLine();
    if (ImGui::Button("Teleport Waypoint")) { std::strncpy(g_state.configStatus, "Teleport: Waypoint (demo)", sizeof(g_state.configStatus)); }
}

void drawTabWorld() {
    ImGui::SliderFloat("Time of Day", &g_state.worldTime, 0.0f, 24.0f, "%.1f h");
    ImGui::SliderFloat("Gravity", &g_state.worldGravity, 0.0f, 30.0f);
    ImGui::Checkbox("Fog", &g_state.worldFog);
    ImGui::Checkbox("No Fog", &g_state.worldNoFog);
    ImGui::SliderFloat("Camera FOV", &g_state.worldFov, 60.0f, 120.0f);
    ImGui::SliderFloat("Brightness", &g_state.worldBrightness, 0.2f, 3.0f);
}

void drawTabWeapon() {
    ImGui::Columns(2, nullptr, false);
    ImGui::Checkbox("No Spread", &g_state.wpnNoSpread);
    ImGui::Checkbox("Fast Reload", &g_state.wpnFastReload);
    ImGui::Checkbox("Magic Bullet", &g_state.wpnMagic);
    ImGui::NextColumn();
    ImGui::Checkbox("Infinite Ammo", &g_state.wpnInfAmmo);
    ImGui::SliderFloat("Fire Rate", &g_state.wpnFireRate, 0.5f, 5.0f, "x%.2f");
    ImGui::Columns(1);
}

void drawTabMisc() {
    ImGui::Checkbox("Custom Crosshair", &g_state.miscCrosshair);
    ImGui::ColorEdit4("Crosshair Color", g_state.miscCrossColor, ImGuiColorEditFlags_NoInputs);
    ImGui::Checkbox("FPS Counter Overlay", &g_state.miscFps);
    g_state.showFpsOverlay = g_state.miscFps;
    ImGui::Checkbox("Watermark", &g_state.miscWatermark);
    g_state.showWatermark = g_state.miscWatermark;
    ImGui::Checkbox("Bypass Toggle", &g_state.miscBypass);
    ImGui::Checkbox("Anti-AFK", &g_state.miscAntiAfk);
    ImGui::InputText("Spoof Name", g_state.spoofName, sizeof(g_state.spoofName));
}

void drawTabSettings() {
    if (ImGui::ColorEdit4("Accent Color", g_state.accent, ImGuiColorEditFlags_AlphaBar)) g_accentDirty = true;
    if (ImGui::SliderFloat("Menu Scale", &g_state.menuScale, 0.85f, 1.35f, "%.2f")) applyCombinedScale(combinedUiScale());
    if (ImGui::SliderFloat("Window Opacity", &g_state.windowAlpha, 0.5f, 1.0f)) g_accentDirty = true;
    if (g_accentDirty) applyAccentColors();
    const char *keys[] = {"Insert", "Home", "F1", "Volume Up"};
    ImGui::Combo("Menu Toggle Key", &g_state.menuKey, keys, IM_ARRAYSIZE(keys));
    if (ImGui::Button("Reset Style")) { g_state.menuScale = 1.0f; g_state.accent[0] = 0.902f; g_state.accent[1] = 0.098f; g_state.accent[2] = 0.098f; g_state.accent[3] = 1.0f; g_themeReady = false; updateDisplayScale(g_dispW, g_dispH); applyAccentColors(); }
    ImGui::SameLine();
    if (ImGui::Button("Close Menu")) g_state.menuOpen = false;
}

void drawTabConfig() {
    const char *slots[] = {"Slot 1", "Slot 2", "Slot 3", "Slot 4", "Slot 5"};
    ImGui::Combo("Config Slot", &g_state.configSlot, slots, IM_ARRAYSIZE(slots));
    ImGui::InputText("Profile Name", g_state.profileName, sizeof(g_state.profileName));
    if (ImGui::Button("Save")) snprintf(g_state.configStatus, sizeof(g_state.configStatus), "Saved %s to %s", g_state.profileName, slots[g_state.configSlot]);
    ImGui::SameLine();
    if (ImGui::Button("Load")) snprintf(g_state.configStatus, sizeof(g_state.configStatus), "Loaded %s from %s", g_state.profileName, slots[g_state.configSlot]);
    ImGui::TextWrapped("Status: %s", g_state.configStatus);
}

void drawSidebarAndContent(float menuWinW) {
    const char *tabs[] = {"AIMBOT", "VISUALS", "PLAYER", "WORLD", "WEAPON", "MISC", "SETTINGS", "CONFIG"};
    float s = combinedUiScale();
    float sidebarW = 148.0f * s;
    if (sidebarW > menuWinW * 0.32f) sidebarW = menuWinW * 0.32f;
    if (sidebarW < 112.0f) sidebarW = 112.0f;
    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, 0.0f), true);
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
    for (int i = 0; i < IM_ARRAYSIZE(tabs); i++) {
        bool sel = (g_state.activeTab == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(g_state.accent[0], g_state.accent[1], g_state.accent[2], 0.28f));
        if (ImGui::Selectable(tabs[i], sel, 0, ImVec2(0.0f, 36.0f * s))) g_state.activeTab = i;
        if (sel) ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##content", ImVec2(0.0f, 0.0f), true);
    switch (g_state.activeTab) {
        case 0: drawTabAimbot(); break;
        case 1: drawTabVisuals(); break;
        case 2: drawTabPlayer(); break;
        case 3: drawTabWorld(); break;
        case 4: drawTabWeapon(); break;
        case 5: drawTabMisc(); break;
        case 6: drawTabSettings(); break;
        case 7: drawTabConfig(); break;
        default: break;
    }
    ImGui::EndChild();
}

bool drawMainMenu() {
    float s = combinedUiScale();
    float winW = kMenuBaseW * s;
    float winH = kMenuBaseH * s;
    if (g_dispW > 0 && g_dispH > 0) fitMenuSizeToScreen(&winW, &winH, (float)g_dispW * 0.92f, (float)g_dispH * 0.88f);
    if (g_dispW > 0 && g_dispH > 0) ImGui::SetNextWindowPos(ImVec2(((float)g_dispW - winW) * 0.5f, ((float)g_dispH - winH) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGuiWindowFlags wf = ImGuiWindowFlags_NoCollapse;
    bool open = g_state.menuOpen;
    if (!ImGui::Begin("Android-Attack Mod Menu", &open, wf)) {
        ImGui::End();
        g_state.menuOpen = open;
        return false;
    }
    g_state.menuOpen = open;
    drawSwissHeader();
    drawSidebarAndContent(winW);
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    touchInput::setCaptureRect(pos.x, pos.y, size.x, size.y);
    ImGui::End();
    return true;
}

}

static bool g_fontsReady = false;
static bool g_glFontsReady = false;

void setupFonts() {
    if (g_fontsReady) return;
    ImGuiIO &io = ImGui::GetIO();
    io.FontGlobalScale = 1.0f;
    io.Fonts->Clear();
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;
    cfg.RasterizerMultiply = 1.0f;
    cfg.RasterizerDensity = 2.0f;
    cfg.SizePixels = 30.0f;
    io.Fonts->AddFontFromMemoryTTF((void*)overlayFont::robotoMediumData(), overlayFont::robotoMediumSize(), cfg.SizePixels, &cfg, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->TexGlyphPadding = 2;
    io.Fonts->Build();
    g_fontsReady = true;
}

void onGlRendererReady() {
    g_glFontsReady = true;
    if (g_fontsReady) ImGui_ImplOpenGL3_CreateFontsTexture();
}

void initTheme() {
    if (g_dispW > 0 && g_dispH > 0) updateDisplayScale(g_dispW, g_dispH);
    else applyCombinedScale(2.0f * g_state.menuScale);
    g_themeReady = true;
    applyAccentColors();
}

void draw(int displayWidth, int displayHeight) {
    updateDisplayScale(displayWidth, displayHeight);
    if (g_accentDirty) applyAccentColors();
    drawWatermark();
    drawFovCircle(displayWidth, displayHeight);
    drawMiscCrosshair(displayWidth, displayHeight);
    if (g_state.menuOpen) {
        drawMainMenu();
    } else {
        touchInput::clearCaptureRect();
        drawMenuToggleButton();
    }
}

}
