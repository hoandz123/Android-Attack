#pragma once

#include <API/Il2CppApi.h>

namespace TableFishingDifficultyImpl {

Class *get_class();
Object *get_Instance();
Object *GetTableData(unsigned int sid);
int ShadowIndexFromAssetName(const char *name);
const char *ShadowLabelFromIndex(int index);
bool Query(unsigned int sid, int *outShadowIndex, unsigned int *outDifficultyId);

}
