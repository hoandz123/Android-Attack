#include "RollCongCu.h"
#include "Config/Config.h"
#include <API/Il2CppApi.h>
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>
#include <Tools/Tools.h>
#include <sstream>
#include <string>
#include <vector>

namespace ItemAffixOptionView_NS {

namespace {

void *g_thiz = nullptr;

List<void *> *OptionList() {
    return ((Object *) g_thiz)->get_field_object<List<void *> *>(OBF("_optionList"));
}

void (*old_UpdateAffixCost)(void *);
void UpdateAffixCost(void *thiz) {
    g_thiz = thiz;
    old_UpdateAffixCost(thiz);
}

int GetGrade(int id) {
    List<void *> *list = OptionList();
    if (!list) return -1;
    for (int i = 0; i < list->get_Count(); ++i) {
        Object *item = (Object *) list->get_item(i);
        if (!item) continue;
        if (item->get_field_value<int>(OBF("OptionId")) == id) return item->get_field_value<int>(OBF("Grade"));
    }
    return 0;
}

struct ParsedItem {
    int id = 0;
    bool isLock = false;
};

std::vector<ParsedItem> ParseItems(const std::string &s) {
    std::vector<ParsedItem> out;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, '#')) {
        if (token.empty()) continue;
        auto colon = token.find(':');
        auto bar = token.find('|', colon + 1);
        if (colon == std::string::npos || bar == std::string::npos) continue;
        ParsedItem item;
        item.id = std::stoi(token.substr(colon + 1, bar - colon - 1));
        item.isLock = token[bar + 1] != '0';
        out.push_back(item);
    }
    return out;
}

namespace RequestItemAffixState {
void *thiz = nullptr;
long uid = 0;
void *lockingIndexs = nullptr;
bool openShop = false;
bool checkHighGrade = false;
bool useLock = false;
}

void (*old_RequestItemAffix)(void *, long, void *, bool, bool, bool);
void RequestItemAffix(void *thiz, long uid, void *lockingIndexs, bool openShop, bool checkHighGrade, bool useLock) {
    old_RequestItemAffix(thiz, uid, lockingIndexs, openShop, checkHighGrade, useLock);
    RequestItemAffixState::thiz = thiz;
    RequestItemAffixState::uid = uid;
    RequestItemAffixState::lockingIndexs = lockingIndexs;
    RequestItemAffixState::openShop = openShop;
    RequestItemAffixState::checkHighGrade = checkHighGrade;
    RequestItemAffixState::useLock = useLock;
}

bool (*old_CheckUnlockHighGrade)(void *, void *, void *, int);
bool CheckUnlockHighGrade(void *thiz, void *userItem, void *lockingIndexs, int grade) {
    if (gPLConfig.fishing.RollCapDo > 0) return false;
    return old_CheckUnlockHighGrade(thiz, userItem, lockingIndexs, grade);
}

void *(*old_Deserialize)(void *, void **, void **);
void *Deserialize(void *thiz, void **reader, void **options) {
    void *ret = old_Deserialize(thiz, reader, options);
    if (ret) {
        String *str = ((Object *) ret)->get_field_object<String *>(OBF("<NewAdditions>k__BackingField"));
        if (str) {
            std::vector<ParsedItem> items = ParseItems(str->to_string());
            bool isValid = true;
            for (size_t i = 0; i < items.size(); ++i) {
                ParsedItem item = items[i];
                int grade = GetGrade(item.id);
                if (!item.isLock && grade >= gPLConfig.fishing.RollCapDo) isValid = false;
            }
            if (isValid && RequestItemAffixState::thiz) {
                RequestItemAffix(RequestItemAffixState::thiz, RequestItemAffixState::uid, RequestItemAffixState::lockingIndexs, RequestItemAffixState::openShop, RequestItemAffixState::checkHighGrade, RequestItemAffixState::useLock);
            }
        }
    }
    return ret;
}

}

void init() {
    Tools::Hook((void *) GET_METHOD("ItemAffixOptionView", "UpdateAffixCost", 0), (void *) UpdateAffixCost, (void **) &old_UpdateAffixCost);
    Tools::Hook((void *) GET_METHOD("ItemOptionAffixingAFormatter", "Deserialize", 2), (void *) Deserialize, (void **) &old_Deserialize);
    Tools::Hook((void *) GET_METHOD("CombineSystem", "RequestItemAffix", 5), (void *) RequestItemAffix, (void **) &old_RequestItemAffix);
    Tools::Hook((void *) GET_METHOD("CombineSystem", "CheckUnlockHighGrade", 3), (void *) CheckUnlockHighGrade, (void **) &old_CheckUnlockHighGrade);
}

}
