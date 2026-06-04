#pragma once

#include <functional>
#include <string>
#include <API/Vector3.h>
#include <imgui.h>

namespace EspGUI {

constexpr ImU32 kEspLineColor = IM_COL32(255, 60, 60, 220);
constexpr float kDefaultLineThickness = 2.f;

bool projectSegment(const Vector3 &from, const Vector3 &to, float sw, float sh, float outFrom[2], float outTo[2]);

struct PosSmoother {
    static constexpr int kSlots = 32;
    static constexpr unsigned int kMyKey = 0xFFFFFFFFu;
    static constexpr float kTau = 0.09f;
    static constexpr float kSnapDist = 8.0f;
    Vector3 get(unsigned int objId, const Vector3 &target, float dt);
    void tick(float dt);
    unsigned int keyFor(unsigned int objId) const;

private:
    struct Slot { unsigned int objId = 0; Vector3 disp{}; bool valid = false; float lastSeen = 0.f; };
    Slot slots[kSlots]{};
    float clock = 0.f;
    int findSlot(unsigned int objId) const;
    int allocSlot(unsigned int objId);
};

void DrawLine(ImVec2 start, ImVec2 end, float thickness, ImVec4 color);
void DrawTooltip(ImVec2 position, const char *text, ImU32 bgColor = IM_COL32(60, 60, 60, 255));
void CircleBtn(const ImVec2 &pos, const Vector3 &cbPos, float radius, const std::function<void(const Vector3 &)> &cb, const char *label = nullptr, bool filled = false, ImU32 col = IM_COL32(255, 255, 255, 255), ImU32 textCol = IM_COL32(255, 255, 255, 255));

}
