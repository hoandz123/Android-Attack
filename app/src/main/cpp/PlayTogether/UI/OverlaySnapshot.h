#pragma once

#include <API/Vector3.h>

namespace OverlaySnapshot {

struct View {
    bool ready = false;
    int mapId = 0;
    Vector3 position{};
    int fishCaught = 0;
    int fishingState = 0;
};

void UpdateFromGameThread();
void Read(View &out);
const char *FishingStateLabel(int fishingState);

}
