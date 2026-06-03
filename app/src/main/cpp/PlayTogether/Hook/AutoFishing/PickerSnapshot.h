#pragma once

namespace AutoFishing {

constexpr int kPickerMaxBaits = 64;
constexpr int kPickerMaxZones = 128;
constexpr int kPickerMaxBaitRecipes = 96;
constexpr int kPickerLabelLen = 96;

struct PickerBaitEntry {
    unsigned int itemId = 0;
    char label[kPickerLabelLen]{};
};

struct PickerBaitRecipeEntry {
    unsigned int itemId = 0;
    unsigned int recipeId = 0;
    int ownedCount = 0;
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
    int baitRecipeCount = 0;
    PickerBaitRecipeEntry baitRecipes[kPickerMaxBaitRecipes]{};
    int zoneCount = 0;
    PickerZoneEntry zones[kPickerMaxZones]{};
};

void UpdatePickerFromGameThread();
void InitPickerCache();
void RequestPickerRebuild();
void NotifyPickerOpen();
void NotifyCraftPanelVisible();
void NotifyPickerClosed();
void ReadPicker(PickerSnapshot &out);
unsigned int LookupBaitCraftRecipeId(unsigned int itemId);

}
