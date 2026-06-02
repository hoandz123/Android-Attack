#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <API/Vector2.h>
#include <API/Vector3.h>
#include <imgui.h>

struct ESPEntry {
    void *object = nullptr;
    Vector2 startPos = Vector2();
    Vector3 pos;
    Vector3 screenPos;
    std::string name;
    ImVec4 color = ImVec4(1, 1, 1, 1);
    int staleCount = 0;

    ESPEntry() = default;
    ESPEntry(void *obj, const Vector3 &wp, const Vector3 &sp, const std::string &n) : object(obj), pos(wp), screenPos(sp), name(n), staleCount(0) {}
};

class ESPManager {
public:
    static void Add(ESPEntry entry) {
        entries()[entry.object] = entry;
    }

    static void Add(void *obj, const Vector3 &pos, const Vector3 &screenPos, const std::string &name = "", const ImVec4 &color = ImVec4(1, 1, 1, 1)) {
        auto &map = entries();
        auto it = map.find(obj);
        if (it != map.end()) {
            it->second.pos = pos;
            it->second.screenPos = screenPos;
            it->second.name = name;
            it->second.staleCount = 0;
            it->second.color = color;
        } else {
            map[obj] = ESPEntry(obj, pos, screenPos, name);
        }
    }

    static std::unordered_map<void *, ESPEntry> GetEntries() {
        auto &map = entries();
        for (auto it = map.begin(); it != map.end(); ) {
            it->second.staleCount++;
            if (it->second.staleCount > 60) {
                it = map.erase(it);
            } else {
                ++it;
            }
        }
        return map;
    }

    static ESPEntry GetEntry(void *obj) {
        auto &map = entries();
        auto it = map.find(obj);
        if (it != map.end()) return it->second;
        return ESPEntry();
    }

    static void Clear() {
        std::lock_guard<std::mutex> lock(mutex());
        entries().clear();
    }

private:
    static std::unordered_map<void *, ESPEntry> &entries() {
        static std::unordered_map<void *, ESPEntry> instance;
        return instance;
    }

    static std::mutex &mutex() {
        static std::mutex m;
        return m;
    }
};
