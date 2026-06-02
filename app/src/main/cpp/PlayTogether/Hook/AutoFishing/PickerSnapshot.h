#pragma once

namespace AutoFishing {

constexpr int kPickerMaxBaits = 64;
constexpr int kPickerMaxZones = 128;
constexpr int kPickerLabelLen = 96;

struct PickerBaitEntry {
    unsigned int itemId = 0;
    char label[kPickerLabelLen]{};
};

struct PickerZoneEntry {
    unsigned int zoneId = 0;
    char label[kPickerLabelLen]{};
};

struct PickerSnapshot {
    bool ready = false;
    int baitCount = 0;
    PickerBaitEntry baits[kPickerMaxBaits]{};
    int zoneCount = 0;
    PickerZoneEntry zones[kPickerMaxZones]{};
};

void UpdatePickerFromGameThread();
void RequestPickerRebuild();
void NotifyPickerOpen();
void ReadPicker(PickerSnapshot &out);

}
