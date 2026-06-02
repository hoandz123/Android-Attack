#include "EspGUI.h"
#include <imgui.h>

namespace EspGUI {

void DrawLine(ImVec2 start, ImVec2 end, float thickness, ImVec4 color) {
    ImDrawList *background = ImGui::GetBackgroundDrawList();
    if (!background) return;
    background->AddLine(start, end, ImColor(color.x, color.y, color.z, color.w), thickness);
}

void DrawTooltip(ImVec2 position, const char *text, ImU32 bgColor) {
    ImDrawList *drawList = ImGui::GetBackgroundDrawList();
    if (!drawList || !text || !text[0]) return;
    float fontSize = 15.f;
    ImVec2 textSize = ImGui::CalcTextSize(text);
    float pad = fontSize * 0.5f;
    ImVec2 rectMin = position;
    ImVec2 rectMax = ImVec2(position.x + textSize.x + pad * 2.f, position.y + textSize.y + pad * 2.f);
    drawList->AddRectFilled(rectMin, rectMax, bgColor, 10.f);
    drawList->AddRect(rectMin, rectMax, IM_COL32(255, 255, 255, 255), 10.f);
    drawList->AddText(nullptr, fontSize, ImVec2(position.x + pad, position.y + pad), IM_COL32(255, 255, 255, 255), text);
}

void CircleBtn(const ImVec2 &pos, const Vector3 &cbPos, float radius, const std::function<void(const Vector3 &)> &cb, const char *label, bool filled, ImU32 col, ImU32 textCol) {
    ImGuiIO &io = ImGui::GetIO();
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;
    ImVec2 p{pos.x, pos.y};
    if (filled) {
        dl->AddCircleFilled(p, radius + 1.f, IM_COL32(255, 255, 255, 255));
    } else {
        dl->AddCircle(p, radius + 1.f, IM_COL32(255, 255, 255, 255), 0, 2.f);
    }
    if (filled) {
        dl->AddCircleFilled(p, radius, col);
    } else {
        dl->AddCircle(p, radius, col);
    }
    if (label && label[0]) {
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p.x - ts.x / 2.f, p.y - ts.y / 2.f), textCol, label);
    }
    if (io.MouseClicked[0]) {
        float dx = io.MousePos.x - p.x;
        float dy = io.MousePos.y - p.y;
        if (dx * dx + dy * dy <= radius * radius && cb) cb(cbPos);
    }
}

}
