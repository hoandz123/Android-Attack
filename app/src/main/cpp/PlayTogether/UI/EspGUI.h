#pragma once

#include <functional>
#include <string>
#include <API/Vector3.h>
#include <imgui.h>

namespace EspGUI {
void DrawLine(ImVec2 start, ImVec2 end, float thickness, ImVec4 color);
void DrawTooltip(ImVec2 position, const char *text, ImU32 bgColor = IM_COL32(60, 60, 60, 255));
void CircleBtn(const ImVec2 &pos, const Vector3 &cbPos, float radius, const std::function<void(const Vector3 &)> &cb, const char *label = nullptr, bool filled = false, ImU32 col = IM_COL32(255, 255, 255, 255), ImU32 textCol = IM_COL32(255, 255, 255, 255));
}
