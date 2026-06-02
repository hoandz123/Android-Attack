#pragma once

#include <string>
#include <unordered_map>
#include <API/Vector3.h>
#include <API/Vector2.h>
#include <imgui/imgui.h>

struct ESPEntry {
    void *object = nullptr;
    Vector2 startPos = Vector2();
    Vector3 pos;
    Vector3 screenPos;
    std::string name;
    ImVec4 color = ImVec4(1, 1, 1, 1);
    int staleCount = 0;
};

class ESPManager {
public:
    static void Add(ESPEntry entry) {}
    static void Add(void *obj, const Vector3 &pos, const Vector3 &screenPos, const std::string &name = "", const ImVec4 &color = ImVec4(1, 1, 1, 1)) {}
    static std::unordered_map<void *, ESPEntry> GetEntries() { return {}; }
};
