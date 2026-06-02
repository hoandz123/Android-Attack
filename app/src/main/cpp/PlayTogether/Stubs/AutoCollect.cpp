//
// Created by TEAMHMG on 13/09/2025.
//
#include "AutoCollect.h"
#include "Config/Config.h"
#include "CacheUser.h"
#include "KinematicCharacterMotor.h"
#include "LayerSystem.h"
#include "TableSystem.h"
#include "DialogJoyStick.h"
#include "DialogActionButtons.h"
#include "Tools/Tools.h"
#include "enum/eTableType.h"
#include "PlayLog.h"


namespace CollectSys {
    static inline unsigned char tolower_uc(unsigned char c) {
        return static_cast<unsigned char>(std::tolower(c));
    }

    static bool ci_find(const std::string &hay, const std::string &needle) {
        if (needle.empty()) return true;
        if (hay.size() < needle.size()) return false;

        auto it = std::search(
                hay.begin(), hay.end(),
                needle.begin(), needle.end(),
                [](char a, char b) {
                    return tolower_uc(static_cast<unsigned char>(a)) ==
                           tolower_uc(static_cast<unsigned char>(b));
                }
        );
        return it != hay.end();
    }


    SpawnType GetSpawnType(const std::string &name) {
        if (name.empty()) return SpawnType::Unknown;

        if (ci_find(name, "bad")) return SpawnType::Unknown;

        static const std::vector<std::pair<std::string, SpawnType>> patterns = {
                {"gemvein", SpawnType::GemVein},
                {"vein", SpawnType::Vein},
                {"plants", SpawnType::Plants},
                {"fossil", SpawnType::Fossil},
                {"slime", SpawnType::Slime},
                {"snowman", SpawnType::Snowman},
                {"ore", SpawnType::Ore},
                {"ing_", SpawnType::Ing},
                {"fishingzone", SpawnType::FishingZone},
                {"gathering", SpawnType::Gathering},
                {"cardcollect", SpawnType::CardCollect},
                {"coin", SpawnType::Coin},
                {"nametag", SpawnType::NameTag},
                {"fishbreadshop", SpawnType::FishBreadShop},
                {"dragonvillage_monster", SpawnType::DragonVillageMonster}
        };

        for (const auto &p: patterns) {
            if (ci_find(name, p.first)) return p.second;
        }
        return SpawnType::Unknown;
    }

    bool IsObjectOfType(const std::string &name, SpawnType type) {
        return GetSpawnType(name) == type;
    }

    std::string GetSpawnTypeName(SpawnType type) {
        switch (type) {
            case SpawnType::Unknown:
                return "Không rõ";
            case SpawnType::Vein:
                return "Mạch khoáng";
            case SpawnType::Plants:
                return "Thực vật";
            case SpawnType::GemVein:
                return "Mạch ngọc";
            case SpawnType::Fossil:
                return "Hóa thạch";
            case SpawnType::Slime:
                return "Slime";
            case SpawnType::Snowman:
                return "Người tuyết";
            case SpawnType::Ore:
                return "Quặng";
            case SpawnType::Ing:
                return "Nguyên liệu";
            case SpawnType::FishingZone:
                return "Khu câu cá";
            case SpawnType::Gathering:
                return "Thu thập";
            case SpawnType::CardCollect:
                return "Thu thập thẻ";
            case SpawnType::Coin:
                return "Xu";
            case SpawnType::NameTag:
                return "Thẻ tên";
            case SpawnType::FishBreadShop:
                return "Tiệm bánh cá";
            case SpawnType::DragonVillageMonster:
                return "Quái thú làng rồng";
            default:
                return "Không rõ";
        }
    }

    std::vector<void *> blockPoiter;

    bool isBlockPointer(void *p) {
        return std::find(blockPoiter.begin(), blockPoiter.end(), p) != blockPoiter.end();
    }

    void themBlock(void *p) { blockPoiter.push_back(p); }

    eAutoState currentState = eAutoState::None;
    Object *currentVein = nullptr;
    Object *currentCollect = nullptr;
    int targetMapId = 0;

    Object *FindCollect();

    bool TeleportToCollect();

    bool isValidCollect(Object *collect);

    Object *FindVein();

    bool TeleportToVein();

    bool isCatchVein();

    bool isValidVein(Object *vein);

    Vector3 posTarget = Vector3::zero();

    void Update() {
        auto &collect = gPLConfig.collect;

        int mapID = CacheUser::myCurrentMapID();

        switch (currentState) {
            case eAutoState::None: {
                if (collect.isAutoDapDa) currentState = eAutoState::FindingVein;
                if (collect.isAutoNguyenLieu) currentState = eAutoState::FindingCollect;
                if (collect.isAutoNhatThe) currentState = eAutoState::FindingCollect;
                break;
            }
            case eAutoState::FindingCollect: {
                if (!collect.isAutoNguyenLieu && !collect.isAutoNhatThe) {
                    LOGI("AutoDapDa: Không bật tính năng tự động nhặt thẻ, chuyển sang trạng thái None");
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(1000);
                currentCollect = FindCollect();
                if (currentCollect) {
                    currentState = eAutoState::TeleportingCollect;
                    LOGI("AutoDapDa: Tìm thấy mỏ, đang dịch chuyển...");
                } else {
                    if (collect.isAutoDapDa) {
                        currentState = eAutoState::FindingVein;
                        LOGI("AutoDapDa: Không tìm thấy mỏ, đang chuyển sang tìm kiếm mỏ đá...");
                    } else {
                        if (collect.isTeleMapCollect) {
                            currentState = eAutoState::NextMap;
                            LOGI("AutoDapDa: Không tìm thấy mỏ, chuyển sang tìm kiếm bản đồ tiếp theo...");
                        } else {
                            currentState = eAutoState::None;
                            LOGI("AutoDapDa: Không tìm thấy mỏ, đang chờ...");
                        }
                    }

                }
                break;
            }
            case eAutoState::TeleportingCollect: {
                if (TeleportToCollect()) {
                    currentState = eAutoState::WaitingForCollect;
                    LOGI("AutoDapDa: Đã dịch chuyển đến mỏ");
                } else {
                    LOGE("AutoDapDa: Không thể dịch chuyển đến mỏ");
                    currentState = eAutoState::FindingCollect;
                }
                break;
            }
            case eAutoState::WaitingForCollect: { // Dịch chuyển tới chờ code nhặt chạy
                if (!collect.isAutoNguyenLieu && !collect.isAutoNhatThe) {
                    LOGI("AutoDapDa: Không bật tính năng tự động nhặt thẻ, chuyển sang trạng thái None");
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(5000);
                if (posTarget != Vector3::zero() &&
                    Vector3::Distance(KinematicCharacterMotor::get_TransientPosition(), posTarget) <
                    1.5f) {
                    posTarget = Vector3::zero();
                    LOGI("[WaitingForCard] Kiểm tra đã bắt được thẻ...");
                } else {
                    LOGI("[WaitingForCard] Chưa đến vị trí thẻ, đang chờ...");
                }
                currentCollect = nullptr;
                currentState = eAutoState::FindingCollect;
                break;
            }
            case eAutoState::FindingVein: {
                if (!collect.isAutoDapDa) {
                    LOGI("AutoDapDa: Không bật tính năng tự động đào đá, chuyển sang trạng thái None");
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(1000);
                currentVein = FindVein();
                if (currentVein) {
                    currentState = eAutoState::Teleporting;
                    LOGI("AutoDapDa: Tìm thấy mỏ, đang dịch chuyển...");
                } else {
                    if (collect.isTeleMapCollect) {
                        currentState = eAutoState::NextMap;
                        LOGI("AutoDapDa: Không tìm thấy mỏ, chuyển sang tìm kiếm bản đồ tiếp theo...");
                    } else {
                        currentState = eAutoState::None;
                        LOGI("AutoDapDa: Không tìm thấy mỏ, đang chờ...");
                    }
                }
                break;
            }
            case eAutoState::Teleporting: {
                if (TeleportToVein()) {
                    currentState = eAutoState::WaitingForTeleport;
                    LOGI("AutoDapDa: Đã dịch chuyển đến mỏ");
                } else {
                    LOGE("AutoDapDa: Không thể dịch chuyển đến mỏ");
                    currentState = eAutoState::FindingVein;
                }
                break;
            }
            case eAutoState::WaitingForTeleport: {
                RATE_LIMIT(1000);
                currentState = eAutoState::CatchingVein;
                LOGI("AutoDapDa: Đã đến vị trí mỏ, bắt đầu thu thập");
                break;
            }
            case eAutoState::CatchingVein: {
                if (!collect.isAutoDapDa) {
                    LOGI("AutoDapDa: Không bật tính năng tự động đào đá, chuyển sang trạng thái None");
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(collect.delayDapDa);
                if (isCatchVein()) {
                    LOGI("AutoDapDa: Đã bắt được mỏ, đang chờ thu thập xong...");
                } else {
                    LOGE("AutoDapDa: Không thể bắt được mỏ, đang thử lại...");
                    currentVein = nullptr;
                    currentState = eAutoState::FindingVein;
                }
                break;
            }
            case eAutoState::NextMap: { // Chuyển sang bản đồ tiếp theo
                RATE_LIMIT(collect.delayNextMap);
                if (collect.isAutoDapDa && FindVein() != nullptr) {
                    LOGI("AutoDapDa::Update: Đã tìm thấy mỏ đá hợp lệ, không cần chuyển sang bản đồ tiếp theo");
                    currentState = eAutoState::None;
                    return;
                }
                if ((collect.isAutoNguyenLieu || collect.isAutoNhatThe) &&
                    FindCollect() != nullptr) {
                    LOGI("AutoDapDa::Update: Đã tìm thấy mỏ nguyên liệu hợp lệ, không cần chuyển sang bản đồ tiếp theo");
                    currentState = eAutoState::None;
                    return;
                }

                if (gPLConfig.collect.DSMapDa.empty()) {
                    LOGE("DSMapDa trống, không thể chuyển sang bản đồ tiếp theo");
                    currentState = eAutoState::None;
                    return;
                }

                auto &ds = gPLConfig.collect.DSMapDa;
                auto it = ds.upper_bound(mapID);
                it = std::find_if(it, ds.end(), [](auto &kv) {
                    return kv.second;
                });
                if (it == ds.end())
                    it = std::find_if(ds.begin(), ds.end(), [](auto &kv) {
                        return kv.second;
                    });
                if (it != ds.end()) {
                    targetMapId = it->first;

                    LayerSystem::NextMap(targetMapId);
                    currentState = eAutoState::WaitingForNextMap;
                } else {
                    LOGE("AutoInsectCatcher::Update: Không tìm thấy bản đồ hợp lệ trong DSMapDa");
                    currentState = eAutoState::None;
                }
                break;
            }
            case eAutoState::WaitingForNextMap: { // Chờ chuyển sang bản đồ mới
                RATE_LIMIT(5000);
                if (mapID == targetMapId) {
                    LOGI("AutoInsectCatcher::Update: Đã chuyển sang bản đồ mới: %d", targetMapId);
                    currentState = eAutoState::None;
                } else {
                    LOGE("AutoInsectCatcher::Update: Chuyển sang bản đồ mới không thành công, đang thử lại...");
                    currentState = eAutoState::NextMap;
                }
                break;
            }

        }
    }

    Object *FindVein() {
        List<Object *> *mapObjectInfoList = CollectSystem::_mapObjectInfoList();
        if (!mapObjectInfoList) return nullptr;

        Object *TItem = TableSystem::GetTableUnit<Object *>(
                FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>(
                        "SpawnObjectList"));
        if (!TItem) {
            LOGE("TableItemImpl is NULL");
            return nullptr;
        }

        Dictionary<int, Object *> *_container = TItem->get_field_object<Dictionary<int, Object *> *>(
                "_container");
        if (!_container) {
            LOGE("_container is NULL");
            return nullptr;
        }

        for (int i = 0; i < mapObjectInfoList->get_Count(); i++) {
            Object *objInfo = mapObjectInfoList->get_item(i);
            if (!objInfo) continue;

            if (isBlockPointer(objInfo)) {
                continue;
            }

            Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
            if (!MapObjectInfo) continue;


            int ResourceId = MapObjectInfo->invoke_method<int>("get_ResourceId");
            int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
            uint8_t Step = MapObjectInfo->invoke_method<uint8_t>("get_Step");
            if (Step == 0) continue; // Bỏ qua các mỏ đã thu

            std::vector<int16_t> blockSpawnPointId = {-22231, -22201, -15051, 991, 993, 995, 997,
                    999, 2002, 4873, 30001, 30016, 31107, 31127, 31167, 31203, 21233, 21235, 22001};
            if (std::find(blockSpawnPointId.begin(), blockSpawnPointId.end(), SpawnPointId) !=
                blockSpawnPointId.end()) {
                continue; // Bỏ qua các góc lag
            }


            Object *SpawnObjectList = _container->get_Item(ResourceId);
            if (!SpawnObjectList) continue;

            String *AssetName = SpawnObjectList->get_field_object<String *>(
                    "<AssetName>k__BackingField");
            if (!AssetName) continue;

            std::string AssetNameStr = AssetName->to_string();

            if (AssetNameStr.find("bad") != std::string::npos) {
                continue;
            }
            if (ResourceId == 1085) {
                continue; // Bỏ qua cây halloween lớn
            }
            auto it = gPLConfig.collect.DSTypeDa.find(GetSpawnType(AssetNameStr));
            if (it != gPLConfig.collect.DSTypeDa.end() && it->second) {
                LOGI("Collect Info: Tên mỏ: %s, Loại mỏ: %s, ResourceId: %d, SpawnPointId: %d, Step: %d",
                     AssetNameStr.c_str(),
                     GetSpawnTypeName(GetSpawnType(AssetNameStr)).c_str(),
                     ResourceId,
                     SpawnPointId,
                     Step);

                return objInfo != currentVein ? objInfo : nullptr;
            }
        }
        LOGE("AutoDapDa::FindVein: Không tìm thấy mỏ đá hợp lệ");
        return nullptr;
    }

    Object *FindCollect() {
        auto &collect = gPLConfig.collect;

        List<Object *> *mapObjectInfoList = CollectSystem::_mapObjectInfoList();
        if (!mapObjectInfoList) return nullptr;

        Object *TItem = TableSystem::GetTableUnit<Object *>(
                FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>(
                        "SpawnObjectList"));
        if (!TItem) {
            LOGE("TableItemImpl is NULL");
            return nullptr;
        }

        Dictionary<int, Object *> *_container = TItem->get_field_object<Dictionary<int, Object *> *>(
                "_container");
        if (!_container) {
            LOGE("_container is NULL");
            return nullptr;
        }

        for (int i = 0; i < mapObjectInfoList->get_Count(); i++) {
            Object *objInfo = mapObjectInfoList->get_item(i);
            if (!objInfo) continue;

            Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
            if (!MapObjectInfo) continue;


            int ResourceId = MapObjectInfo->invoke_method<int>("get_ResourceId");
            int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
            uint8_t Step = MapObjectInfo->invoke_method<uint8_t>("get_Step");
            if (Step == 0) continue; // Bỏ qua các mỏ đã thu

            std::vector<int16_t> blockSpawnPointId = {-22231, -22201, -15051, 991, 993, 995, 997,
                    999, 2002, 4873, 30001, 30016, 31107, 31127, 31167, 31203, 21233, 21235, 22001};
            if (std::find(blockSpawnPointId.begin(), blockSpawnPointId.end(), SpawnPointId) !=
                blockSpawnPointId.end()) {
                continue; // Bỏ qua các góc lag
            }


            Object *SpawnObjectList = _container->get_Item(ResourceId);
            if (!SpawnObjectList) continue;

            String *AssetName = SpawnObjectList->get_field_object<String *>(
                    "<AssetName>k__BackingField");
            if (!AssetName) continue;

            std::string AssetNameStr = AssetName->to_string();
            if (collect.isAutoNguyenLieu && GetSpawnType(AssetNameStr) == SpawnType::Ing) {
                return objInfo != currentCollect ? objInfo : nullptr;
            }
            if (collect.isAutoNhatThe && GetSpawnType(AssetNameStr) == SpawnType::CardCollect) {
                return objInfo != currentCollect ? objInfo : nullptr;
            }
        }
        LOGE("AutoDapDa::FindCollect: Không tìm thấy mỏ nguyên liệu hợp lệ");
        return nullptr;
    }

    bool TeleportToCollect() {
        if (!currentCollect || !isValidCollect(currentCollect)) {
            LOGE("AutoDapDa::TeleportToCollect: Mỏ không hợp lệ");
            return false;
        }
        Object *tf = currentCollect->invoke_method<Object *>("get_transform");
        if (!tf) {
            LOGE("AutoDapDa::TeleportToCollect: Transform không hợp lệ");
            return false;
        }

        Vector3 pos = tf->invoke_method<Vector3>("get_position");
        LOGI("TeleportToCollect: %s", pos.to_string().c_str());
        if (PLConfig::GetPlayerMapID() == 1001) {
            if (static_cast<int>(pos.x) == -13) {
                pos = Vector3(-13.154480, 2, -18.444519);
            }
            if (static_cast<int>(pos.x) == -15) {
                pos = Vector3(-15.759705, 2, -18.863586);
            }
        }
        if (posTarget != Vector3::zero()) {
            KinematicCharacterMotor::set_TransientPosition(Vector3(pos.x, pos.y, pos.z));
            DialogJoyStick::OnPress_JumpButton();
        } else {
            KinematicCharacterMotor::set_TransientPosition(Vector3(pos.x, pos.y + 3.0f, pos.z));
        }
        posTarget = pos;
        return true;
    }

    bool isValidCollect(Object *collect) {
        List<Object *> *mapObjectInfoList = CollectSystem::_mapObjectInfoList();
        if (!mapObjectInfoList) return false;

        Object *TItem = TableSystem::GetTableUnit<Object *>(
                FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>(
                        "SpawnObjectList"));
        if (!TItem) {
            LOGE("TableItemImpl is NULL");
            return false;
        }

        Dictionary<int, Object *> *_container = TItem->get_field_object<Dictionary<int, Object *> *>(
                "_container");
        if (!_container) {
            LOGE("_container is NULL");
            return false;
        }

        for (int i = 0; i < mapObjectInfoList->get_Count(); i++) {
            Object *objInfo = mapObjectInfoList->get_item(i);
            if (!objInfo) continue;

            Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
            if (!MapObjectInfo) continue;


            int ResourceId = MapObjectInfo->invoke_method<int>("get_ResourceId");
            int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
            uint8_t Step = MapObjectInfo->invoke_method<uint8_t>("get_Step");
            if (Step == 0) continue; // Bỏ qua các mỏ đã thu

            std::vector<int16_t> blockSpawnPointId = {-22231, -22201, -15051, 991, 993, 995, 997,
                    999, 2002, 4873, 30001, 30016, 31107, 31127, 31167, 31203, 21233, 21235, 22001};
            if (std::find(blockSpawnPointId.begin(), blockSpawnPointId.end(), SpawnPointId) !=
                blockSpawnPointId.end()) {
                continue; // Bỏ qua các góc lag
            }


            Object *SpawnObjectList = _container->get_Item(ResourceId);
            if (!SpawnObjectList) continue;

            String *AssetName = SpawnObjectList->get_field_object<String *>(
                    "<AssetName>k__BackingField");
            if (!AssetName) continue;

            std::string AssetNameStr = AssetName->to_string();
            if (GetSpawnType(AssetNameStr) == SpawnType::Ing ||
                GetSpawnType(AssetNameStr) == SpawnType::CardCollect) {
                if (objInfo == collect) {
                    return true; // Mỏ hợp lệ
                }
            }

        }
        return false; // Không tìm thấy mỏ hợp lệ
    }

    bool TeleportToVein() {
        if (!currentVein) {
            LOGE("AutoDapDa::TeleportToVein: Mỏ không hợp lệ");
            return false;
        }
        Object *tf = currentVein->invoke_method<Object *>("get_transform");
        if (!tf) {
            LOGE("AutoDapDa::TeleportToVein: Transform không hợp lệ");
            return false;
        }

        Vector3 pos = tf->invoke_method<Vector3>("get_position");
        KinematicCharacterMotor::set_TransientPosition(Vector3(pos.x, pos.y + 3.5f, pos.z));
        return true;
    }

    bool isCatchVein() {
        if (!currentVein || !isValidVein(currentVein)) {
            LOGE("AutoDapDa::isCatchVein: Mỏ không hợp lệ");
            return false;
        }
        Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
        Object *tf = currentVein->invoke_method<Object *>("get_transform");
        if (!tf) {
            LOGE("AutoDapDa::TeleportToVein: Transform không hợp lệ");
            themBlock(currentVein);
            return false;
        }


        Vector3 veinPos = tf->invoke_method<Vector3>("get_position");
        Vector2 myPosXZ(myPos.x, myPos.z);
        Vector2 veinPosXZ(veinPos.x, veinPos.z);
        float distanceXZ = Vector2::Distance(myPosXZ, veinPosXZ);
        if (distanceXZ < 1.5f) {
            LOGI("AutoDapDa: Đang thu thập mỏ tại vị trí: %s", veinPos.to_string().c_str());
            LOGI("AutoDapDa: Khoảng cách đến mỏ: %.2f", distanceXZ);
            DialogActionButtons::OnClick();
            return true;
        }
        return false;
    }

    bool isValidVein(Object *vein) {
        List<Object *> *mapObjectInfoList = CollectSystem::_mapObjectInfoList();
        if (!mapObjectInfoList) return false;

        Object *TItem = TableSystem::GetTableUnit<Object *>(
                FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>(
                        "SpawnObjectList"));
        if (!TItem) {
            LOGE("TableItemImpl is NULL");
            return false;
        }

        Dictionary<int, Object *> *_container = TItem->get_field_object<Dictionary<int, Object *> *>(
                "_container");
        if (!_container) {
            LOGE("_container is NULL");
            return false;
        }

        for (int i = 0; i < mapObjectInfoList->get_Count(); i++) {
            Object *objInfo = mapObjectInfoList->get_item(i);
            if (!objInfo) continue;

            Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
            if (!MapObjectInfo) continue;


            int ResourceId = MapObjectInfo->invoke_method<int>("get_ResourceId");
            int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
            uint8_t Step = MapObjectInfo->invoke_method<uint8_t>("get_Step");
            if (Step == 0) continue; // Bỏ qua các mỏ đã thu

            std::vector<int16_t> blockSpawnPointId = {-22231, -22201, -15051, 991, 993, 995, 997,
                    999, 2002, 4873, 30001, 30016, 31107, 31127, 31167, 31203, 21233, 21235, 22001};
            if (std::find(blockSpawnPointId.begin(), blockSpawnPointId.end(), SpawnPointId) !=
                blockSpawnPointId.end()) {
                continue; // Bỏ qua các góc lag
            }


            Object *SpawnObjectList = _container->get_Item(ResourceId);
            if (!SpawnObjectList) continue;

            String *AssetName = SpawnObjectList->get_field_object<String *>(
                    "<AssetName>k__BackingField");
            if (!AssetName) continue;

            std::string AssetNameStr = AssetName->to_string();
            auto it = gPLConfig.collect.DSTypeDa.find(GetSpawnType(AssetNameStr));
            if (it != gPLConfig.collect.DSTypeDa.end() && it->second) {
                if (objInfo == vein) {
                    Object *CollectObjectComp = objInfo->get_field_object<Object *>("CollectObjectComp");
                    if (!CollectObjectComp) {
                        LOGE("AutoDapDa::isValidVein: CollectObjectComp không hợp lệ");
                        themBlock(objInfo);
                        return false;
                    }

                    if (!CollectObjectComp->get_field_value<bool>("_isShow")) {
                        LOGE("AutoDapDa::isValidVein: Mỏ chưa sẵn sàng để thu thập");
                        themBlock(objInfo);
                        return false;
                    }

                    Object *CacheTransform = CollectObjectComp->get_field_object<Object *>("CacheTransform");
                    if (!CacheTransform) {
                        LOGE("AutoDapDa::isValidVein: CacheTransform không hợp lệ");
                        themBlock(objInfo);
                        return false;
                    }
                    Object *GameObject = CollectObjectComp->invoke_method<Object *>("get_gameObject");
                    if (!GameObject) {
                        LOGE("AutoDapDa::isValidVein: GameObject không hợp lệ");
                        themBlock(objInfo);
                        return false;
                    }
                    if (!GameObject->invoke_method<bool>("get_active")) {
                        LOGE("AutoDapDa::isValidVein: GameObject không được kích hoạt get_active");
                        themBlock(objInfo);
                        return false;
                    }

                    Object *HitCol = CollectObjectComp->get_field_object<Object *>("HitCol");
                    if (!HitCol) {
                        LOGE("AutoDapDa::isValidVein: HitCol không hợp lệ");
                        themBlock(objInfo);
                        return false;
                    }
                    if (!HitCol->invoke_method<bool>("get_enabled")) {
                        LOGE("AutoDapDa::isValidVein: HitCol không được kích hoạt get_enabled");
                        themBlock(objInfo);
                        return false;
                    }


                    return true; // Mỏ hợp lệ
                }
            }
        }
        LOGE("AutoDapDa::isValidVein: Không tìm thấy mỏ đá hợp lệ");
        return false; // Không tìm thấy mỏ hợp lệ
    }
}

