#pragma once

#include <API/Vector3.h>

namespace OverlaySnapshot {

struct View {
    bool ready = false;
    int mapId = 0;
    Vector3 position{};
};

void UpdateFromGameThread();
void Read(View &out);

}
