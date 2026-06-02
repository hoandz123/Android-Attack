#include "TableFishingDifficultyImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"
#include <cstring>

namespace TableFishingDifficultyImpl {

Class *get_class() {
    return FindClass(OBF("TableFishingDifficultyImpl"));
}

Object *get_Instance() {
    return TableSystem::GetTableUnit(eTableType::FishingDifficulty);
}

Object *GetTableData(unsigned int sid) {
    if (sid == 0) return nullptr;
    Object *impl = get_Instance();
    if (!impl) return nullptr;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return nullptr;
    return impl->invoke_method<Object *>(OBF("GetTableData"), sid);
}

int ShadowIndexFromAssetName(const char *name) {
    if (!name || !name[0]) return 0;
    if (strcmp(name, OBF("fish_s_shadow")) == 0) return 1;
    if (strcmp(name, OBF("fish_m_shadow")) == 0) return 2;
    if (strcmp(name, OBF("fish_l_shadow")) == 0) return 3;
    if (strcmp(name, OBF("fish_xl_shadow")) == 0) return 4;
    if (strcmp(name, OBF("fish_xxl_shadow")) == 0) return 5;
    if (strcmp(name, OBF("fish_xxxl_shadow")) == 0) return 6;
    if (strcmp(name, OBF("fish_4xl_shadow")) == 0) return 7;
    return 0;
}

const char *ShadowLabelFromIndex(int index) {
    switch (index) {
        case 1: return OBF("S");
        case 2: return OBF("M");
        case 3: return OBF("L");
        case 4: return OBF("XL");
        case 5: return OBF("XXL");
        case 6: return OBF("XXXL");
        case 7: return OBF("4XL");
        default: return OBF("?");
    }
}

bool Query(unsigned int sid, int *outShadowIndex, unsigned int *outDifficultyId) {
    Object *row = GetTableData(sid);
    if (!row) return false;
    Class *rowCls = row->get_class();
    if (!rowCls) return false;
    if (outDifficultyId) {
        if (!rowCls->find_method(OBF("get_FishingDifficultyId"), 0)) return false;
        *outDifficultyId = row->invoke_method<unsigned int>(OBF("get_FishingDifficultyId"));
    }
    if (outShadowIndex) {
        *outShadowIndex = 0;
        if (!rowCls->find_method(OBF("get_AssetName"), 0)) return false;
        String *asset = row->invoke_method<String *>(OBF("get_AssetName"));
        if (!asset) return false;
        *outShadowIndex = ShadowIndexFromAssetName(asset->to_string().c_str());
    }
    return true;
}

}
