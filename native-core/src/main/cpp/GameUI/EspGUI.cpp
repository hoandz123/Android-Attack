#include "EspGUI.h"
#include <API/game/UnityEngine/Camera.h>
#include <cmath>
#include <imgui.h>

namespace EspGUI {

bool projectSegment(const Vector3 &from, const Vector3 &to, float sw, float sh, float outFrom[2], float outTo[2]) {
    (void)sw;
    Vector3 sf = UnityEngine::Camera::StaticWorldToScreenPoint(from);
    Vector3 st = UnityEngine::Camera::StaticWorldToScreenPoint(to);
    const float kNear = 0.1f;
    bool fFront = sf.z > kNear;
    bool tFront = st.z > kNear;
    if (!fFront && !tFront) return false;
    Vector3 ef = from;
    Vector3 et = to;
    if (!fFront) {
        float denom = st.z - sf.z;
        if (fabsf(denom) < 1e-4f) return false;
        float t = (kNear - sf.z) / denom;
        ef = from + (to - from) * t;
    }
    if (!tFront) {
        float denom = sf.z - st.z;
        if (fabsf(denom) < 1e-4f) return false;
        float t = (kNear - st.z) / denom;
        et = to + (from - to) * t;
    }
    Vector3 pf = fFront ? sf : UnityEngine::Camera::StaticWorldToScreenPoint(ef);
    Vector3 pt = tFront ? st : UnityEngine::Camera::StaticWorldToScreenPoint(et);
    outFrom[0] = pf.x;
    outFrom[1] = sh - pf.y;
    outTo[0] = pt.x;
    outTo[1] = sh - pt.y;
    return true;
}

unsigned int PosSmoother::keyFor(unsigned int objId) const { return objId ? objId : kMyKey; }

int PosSmoother::findSlot(unsigned int objId) const {
    for (int i = 0; i < kSlots; ++i) if (slots[i].valid && slots[i].objId == objId) return i;
    return -1;
}

int PosSmoother::allocSlot(unsigned int objId) {
    int empty = -1, oldest = 0;
    float oldestTime = 1e9f;
    for (int i = 0; i < kSlots; ++i) {
        if (!slots[i].valid && empty < 0) empty = i;
        if (slots[i].lastSeen < oldestTime) { oldestTime = slots[i].lastSeen; oldest = i; }
    }
    const int idx = empty >= 0 ? empty : oldest;
    slots[idx].objId = objId;
    slots[idx].valid = true;
    slots[idx].lastSeen = clock;
    return idx;
}

void PosSmoother::tick(float dt) { clock += dt; }

Vector3 PosSmoother::get(unsigned int objId, const Vector3 &target, float dt) {
    int idx = findSlot(objId);
    if (idx < 0) {
        idx = allocSlot(objId);
        slots[idx].disp = target;
        return target;
    }
    Slot &s = slots[idx];
    s.lastSeen = clock;
    if (Vector3::Distance(s.disp, target) > kSnapDist) { s.disp = target; return target; }
    const float alpha = 1.f - std::expf(-dt / kTau);
    s.disp = s.disp + (target - s.disp) * alpha;
    return s.disp;
}

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
