#pragma once

#include <imgui.h>

struct ImDrawList;
struct ImVec2;

namespace modui {

/** FAB: poll download (background) + load GL on render thread; cache texture. */
ImTextureID GetFabIcon();

/** Draw FAB icon (rounded) or black placeholder when not ready. */
void DrawFabIcon(ImDrawList *dl, const ImVec2 &p0, const ImVec2 &p1, float rounding);

} // namespace modui
