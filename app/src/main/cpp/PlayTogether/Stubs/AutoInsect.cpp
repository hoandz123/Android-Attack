//
// Created by TEAMHMG on 13/09/2025.
//
//
#include "AutoInsect.h"
#include "Config/Config.h"
#include "CacheSystem.h"
#include "CacheUser.h"
#include "LayerSystem.h"
#include "KinematicCharacterMotor.h"
#include "CollectSystem.h"
#include "TableSystem.h"
#include "AutoCollect.h"
#include "TableItemImpl.h"
#include "DialogActionButtons.h"
#include "InsectSystem.h"
#include "NguiExtensions.h"
#include "ActorControl.h"
#include "Tools/Tools.h"
#include "enum/eTableType.h"
#include "PlayLog.h"
#include <unordered_set>
#include <unwind.h>
#include <dlfcn.h>
#include <map>
#include <vector>
#include <random>


namespace InsectSys {
    bool isStopDiTrenKhong = false;
    bool isDebug = true;
    std::unordered_set<void *> blacklist;

    void addToBlacklist(void *ptr) {
        blacklist.insert(ptr);
    }

    bool isBlacklisted(void *ptr) {
        return blacklist.find(ptr) != blacklist.end();
    }

    List<Object *> *insectList;
    float catchDistance = 1.068878f;
    eAutoState state;
    eSellState sellState;
    Object *currentInsect = nullptr;
    Object *currentCard = nullptr;
    int targetMapId = 0;
    Vector3 posTarget = Vector3::zero();


    std::map<int, std::vector<Vector3>> posList = {
            {1001, // Plaza
                    {
                            Vector3(33.634888, 0.101562, 2.174316),
                            Vector3(32.870850, 0.101562, -16.387695),
                            Vector3(101.091797, -1.716919, 65.287964),
                            Vector3(43.871826, 0.101562, 77.778931),
                            Vector3(-96.406250, 0.101562, 59.880249),
                            Vector3(-70.461914, 0.101562, 34.795349),
                            Vector3(-87.729309, 0.101555, -18.499878),
                            Vector3(-33.738647, 0.101562, -14.256592),
                            Vector3(-38.751831, 0.101562, -71.795715),
                    }
            },
            {1201, // Camping
                    {
                            Vector3(-5.630432, 0.001007, 29.900085),
                            Vector3(39.466431, 0.000977, 39.972656),
                            Vector3(66.535767, 0.000977, 21.541870),
                            Vector3(83.521179, 0.000977, -18.198853),
                            Vector3(50.550171, 0.000977, -40.617493),
                            Vector3(-6.683777, 0.000977, -12.971680),
                            Vector3(-52.939758, 0.000977, -21.135559),
                            Vector3(-34.981995, 0.000992, 24.310974),
                    }
            },
            {1502, // KTT
                    {
                            Vector3(-96.554626, 0.003906, 24.404663),
                            Vector3(-88.287476, -0.098328, 46.500122),
                            Vector3(-94.396057, 2.101685, 103.878662),
                            Vector3(50.193298, 2.101562, 83.751465),
                            Vector3(41.847900, 0.088074, -109.091553),
                            Vector3(32.073730, 0.001892, -166.843262),
                            Vector3(-133.683655, 0.003891, -57.060974),
                            Vector3(140.317261, 2.159958, -58.002563),
                            Vector3(-118.243164, 0.003891, -100.325195),
                            Vector3(97.752441, -0.098328, 34.220703),
                    }
            },
            {1301, // KND
                    {
                            Vector3(118.519897, 1.501007, -2.473877),
                            Vector3(101.398560, 1.500977, -5.861206),
                            Vector3(25.840820, 1.503265, -75.288330),
                            Vector3(64.882507, 1.502991, -119.002869),
                            Vector3(133.388428, 0.986450, -72.432251),
                            Vector3(37.578857, 1.500977, -60.506531),
                    }
            },
            {1401, // Cống ngầm
                    {
                            Vector3(14.508423, 0.001007, 4.985779),
                            Vector3(-12.099976, 0.001007, -5.667236),
                            Vector3(15.868347, 0.000977, 26.011902),
                            Vector3(32.767212, 0.000977, -5.931030),
                            Vector3(50.176025, 0.001007, 20.156921),
                            Vector3(61.162354, 0.001007, -4.053345),
                    }
            },

    };

    Vector3 layViTriNgauNhien() {
        auto it = posList.find(CacheUser::myCurrentMapID());
        if (it == posList.end() || it->second.empty()) return {0, 0, 0};
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<> r(0, it->second.size() - 1);
        return it->second[r(rng)];
    }

    void Update(List<Object *> *_liveInsectList) {
        auto &insect = gPLConfig.insect;
        if (!insect.isAutoBatBo) {
            return;
        }
        insectList = _liveInsectList;
        Object *cacheUser = CacheSystem::get_CacheUser();
        if (!cacheUser) {
            if (isDebug)
                LOGE("Update: CacheUser không hợp lệ");
            return;
        }
        PLConfig::InsectConfig::TotalInsect = CacheUser::GetItemTypeCount(Item_Type::Insect);
        int mapID = CacheUser::myCurrentMapID();
        if (!insectList) {
            return;
        }
        switch (state) {
            case eAutoState::None: {
                if (insect.isBanBo) {
                    int soLuongConTrungCoTheBan = CacheUser::GetCount(Item_Type::Insect,
                                                                      insect.isDuTimTroLen ? 4 : 7);
                    if (soLuongConTrungCoTheBan >= insect.MaxInsectSell ||
                        PLConfig::InsectConfig::TotalInsect > 250) {
                        state = eAutoState::SellInsect;
                        if (isDebug)
                            LOGI("Update: Đang chuyển sang trạng thái bán côn trùng");
                        break;
                    }
                } else if (PLConfig::InsectConfig::TotalInsect == 300) {
                    if (isDebug)
                        LOGI("Update: Đã đạt giới hạn côn trùng");
                    return;
                }
                PLConfig::InsectConfig::isSell = false;
                if (insect.isNhatThe) {
                    state = eAutoState::FindingCard;
                    if (isDebug)
                        LOGI("[FindingCard] Đang tìm kiếm thẻ");
                } else {
                    state = eAutoState::FindingInsect;
                    if (isDebug)
                        LOGI("Update: Đang tìm kiếm côn trùng");
                }
                break;
            }
            case eAutoState::FindingCard: { // Tìm kiếm thẻ côn trùng
                //                    RATE_LIMIT(500);
                currentCard = FindCard();
                if (currentCard) {
                    state = eAutoState::TeleportingCard;
                    isStopDiTrenKhong = true;
                    if (isDebug)
                        LOGI("[FindingCard] Tìm thấy thẻ hợp lệ, đang dịch chuyển...");
                } else {
                    state = eAutoState::FindingInsect;
                    isStopDiTrenKhong = false;
                    if (isDebug)
                        LOGI("[FindingCard] Không tìm thấy thẻ hợp lệ, đang tìm kiếm côn trùng...");
                }
                break;
            }
            case eAutoState::TeleportingCard: { // Dịch chuyển đến thẻ côn trùng
                if (TeleportToCard()) {
                    state = eAutoState::WaitingForCard;
                } else {
                    state = eAutoState::FindingCard;
                }
                break;
            }
            case eAutoState::WaitingForCard: { // Dịch chuyển tới chờ code nhặt chạy
                RATE_LIMIT(5000);
                if (posTarget != Vector3::zero() &&
                    Vector3::Distance(KinematicCharacterMotor::get_TransientPosition(), posTarget) <
                    1.5f) {
                    posTarget = Vector3::zero();
                    if (isDebug)
                        LOGI("[WaitingForCard] Kiểm tra đã bắt được thẻ...");
                } else {
                    if (isDebug)
                        LOGI("[WaitingForCard] Chưa đến vị trí thẻ, đang chờ...");
                }
                state = eAutoState::FindingCard;
                break;
            }
            case eAutoState::FindingInsect: { // Tìm kiếm côn trùng
                //                    RATE_LIMIT(500);
                currentInsect = FindInsect();
                if (currentInsect) {
                    state = eAutoState::Teleporting;
                } else {
                    if (insect.isTeleMapBo) {
                        state = eAutoState::NextMap;
                        if (isDebug)
                            LOGI("[FindingInsect] Không tìm thấy côn trùng hợp lệ, đang chuyển sang bản đồ tiếp theo...");
                    } else {
                        if (isDebug)
                            LOGI("[FindingInsect] Không tìm thấy côn trùng hợp lệ, đang tìm kiếm lại...");
                    }
                }
                break;
            }
            case eAutoState::Teleporting: { // Dịch chuyển đến côn trùng
                if (TeleportToInsect()) {
                    DichChuyenBo();
                    state = eAutoState::WaitingForTeleport;
                } else {
                    state = eAutoState::FindingInsect;
                }
                break;
            }
            case eAutoState::WaitingForTeleport: { // Chờ dịch chuyển hoàn tất
                static int randomDelay = 0;
                if (randomDelay == 0) {
                    randomDelay = rand() % 3000 + 500;
                } // Giới hạn ngẫu nhiên 500ms đến 5000ms
                RATE_LIMIT(insect.delayBatCT + randomDelay);
                randomDelay = 0;
                state = eAutoState::CatchingInsect;
                break;
            }
            case eAutoState::CatchingInsect: { // Bắt côn trùng
                if (isCatchComplete()) {
                    if (isDebug)
                        LOGI("[CatchingInsect] Bắt côn trùng thành công, đang tìm kiếm côn trùng tiếp theo...");
                    state = eAutoState::WaitingForCatch;
                } else {
                    if (isDebug)
                        LOGI("[CatchingInsect] Không thể bắt côn trùng, đang thử lại...");
                    state = eAutoState::None;
                }
                currentInsect = nullptr;
                break;
            }
            case eAutoState::WaitingForCatch: { // Chờ bắt côn trùng hoàn tất
                //                    RATE_LIMIT(500);
                state = eAutoState::None;
                break;
            }
            case eAutoState::NextMap: { // Chuyển sang bản đồ tiếp theo
                RATE_LIMIT(insect.delayTeleMap);
                if (FindInsect() != nullptr) {
                    if (isDebug)
                        LOGI("Update: Đã tìm thấy côn trùng hợp lệ, không cần chuyển sang bản đồ tiếp theo");
                    state = eAutoState::FindingInsect;
                    return;
                }
                if (insect.DSMapBo.empty()) {
                    if (isDebug)
                        LOGE("Update: DSMapBo trống, không thể chuyển sang bản đồ tiếp theo");
                    state = eAutoState::None;
                    return;
                }

                //                    Object *insectSystem = SystemHelper::get_Insect();
                //                    if (insectSystem) {
                //                        insectSystem->set_field_value("_spawnSeq", 0);
                //                    }


                auto itttt = insect.DSMapBo.find(1501);
                if (itttt != insect.DSMapBo.end()) {
                    itttt->second = false;
                }

                auto &ds = insect.DSMapBo;
                auto it = ds.upper_bound(mapID);
                it = std::find_if(it, ds.end(), [](auto &kv) {
                    return kv.second;
                });
                if (it == ds.end()) {
                    it = std::find_if(ds.begin(), ds.end(), [](auto &kv) {
                        return kv.second;
                    });
                }
                if (it != ds.end()) {
                    targetMapId = it->first;
                    LayerSystem::NextMap(targetMapId);
                    state = eAutoState::WaitingForNextMap;
                } else {
                    if (isDebug)
                        LOGE("Update: Không tìm thấy bản đồ hợp lệ trong DSMapBo");
                    state = eAutoState::None;
                }
                break;
            }
            case eAutoState::WaitingForNextMap: { // Chờ chuyển sang bản đồ mới
                RATE_LIMIT(5000);
                if (mapID == targetMapId) {
                    blacklist.clear();
                    if (isDebug)
                        LOGI("Update: Đã chuyển sang bản đồ mới: %d", targetMapId);
                    state = eAutoState::None;
                } else {
                    if (isDebug)
                        LOGE("Update: Chuyển sang bản đồ mới không thành công, đang thử lại...");
                    state = eAutoState::NextMap;
                }
                break;
            }
            case eAutoState::SellInsect: { // Bán côn trùng
                isStopDiTrenKhong = true;
                switch (sellState) {
                    case eSellState::None: {
                        sellState = eSellState::CheckMap;
                        break;
                    }
                    case eSellState::CheckMap: { // Kiểm tra bản đồ
                        if (mapID != 1201) {
                            if (isDebug)
                                LOGI("Update: Không ở bản đồ bán côn trùng, chuyển sang bản đồ bán...");
                            sellState = eSellState::NextMapSell;
                        } else {
                            sellState = eSellState::CheckPosition;
                        }
                        break;
                    }
                    case eSellState::NextMapSell: { // Chuyển sang bản đồ bán côn trùng
                        LayerSystem::NextMap(1201);
                        sellState = eSellState::WaitingForNextMap;
                        break;
                    }
                    case eSellState::WaitingForNextMap: { // Chờ chuyển sang bản đồ bán
                        RATE_LIMIT(5000);
                        if (mapID == 1201) {
                            if (isDebug)
                                LOGI("Update: Đã chuyển sang bản đồ bán côn trùng");
                            sellState = eSellState::CheckPosition;
                        } else {
                            if (isDebug)
                                LOGE("Update: Chuyển sang bản đồ bán không thành công, đang thử lại...");
                            sellState = eSellState::NextMapSell;
                        }
                        break;
                    }
                    case eSellState::CheckPosition: { // Kiểm tra vị trí bán côn trùng
                        RATE_LIMIT(3000);
                        if (Vector3::Distance(KinematicCharacterMotor::get_TransientPosition(),
                                              Vector3(-1.8800048828125, 1.011007080078125,
                                                      -18.29998779296875)) < 5.0f) {
                            if (isDebug)
                                LOGI("Update: Đã ở vị trí bán côn trùng");
                            sellState = eSellState::SellInsect;
                        } else {
                            if (isDebug)
                                LOGE("Update: Không ở vị trí bán côn trùng, đang dịch chuyển...");
                            KinematicCharacterMotor::set_TransientPosition(
                                    Vector3(-1.8800048828125, 5.011007080078125,
                                            -18.29998779296875));
                            sellState = eSellState::CheckPosition;
                        }
                        break;
                    }
                    case eSellState::SellInsect: { // Bán côn trùng
                        CacheUser::ItemSell(Item_Type::Insect, insect.isDuTimTroLen ? 4 : 7);
                        sellState = eSellState::SellComplete;
                        break;
                    }
                    case eSellState::SellComplete: { // Hoàn tất bán côn trùng
                        isStopDiTrenKhong = false;
                        if (isDebug)
                            LOGI("Update: Bán côn trùng hoàn tất, trở về trạng thái None");
                        state = eAutoState::None;
                        sellState = eSellState::None;
                        break;
                    }
                    default: { // Trạng thái không hợp lệ, reset về None
                        if (isDebug)
                            LOGE("Update: Trạng thái không hợp lệ");
                        sellState = eSellState::None;
                        break;
                    }
                }
            }
        }
    }

    Object *FindCard() {
        //        Object *collectSystem = SystemHelper::get_Collect();
        //        if (!collectSystem) {
        //            return nullptr;
        //        }
        //        List<Object*>* list = CollectSystem::_mapObjectInfoList();
        //        if (!list || list->get_Count() < 1) {
        //            return nullptr;
        //        }
        //        for (int i = 0; i < list->get_Count(); i++) {
        //            Object *info = list->get_item(i);
        //            if (!info) {
        //                continue;
        //            }
        //            Object* mapObjectInfo = info->get_field_object<Object*>("MapObjectInfo");
        //            if (!mapObjectInfo) {
        //                continue;
        //            }
        //            uint32_t resourceId = mapObjectInfo->get_field_value<uint32_t>("<ResourceId>k__BackingField");
        //            if (resourceId < 1) {
        //                continue;
        //            }
        //            std::string assetName = TableSpawnObjectListImpl::get_AssetName(resourceId);
        //            if (assetName.empty()) {
        //                continue;
        //            }
        //            if (assetName.find("cardcollect") != std::string::npos) {
        //                return info != currentCard ? info : nullptr;
        //            }
        //        }
        List<Object *> *mapObjectInfoList = CollectSystem::_mapObjectInfoList();
        if (!mapObjectInfoList) {
            return nullptr;
        }

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
            if (!objInfo) {
                continue;
            }

            Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
            if (!MapObjectInfo) {
                continue;
            }


            int ResourceId = MapObjectInfo->invoke_method<int>("get_ResourceId");
            int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
            uint8_t Step = MapObjectInfo->invoke_method<uint8_t>("get_Step");
            if (Step == 0) {
                continue;
            } // Bỏ qua các mỏ đã thu

            std::vector<int16_t> blockSpawnPointId = {-22231, -22201, -15051, 991, 993, 995, 997,
                    999, 2002, 4873, 30001, 30016, 31107, 31127, 31167, 31203, 21233, 21235, 22001};
            if (std::find(blockSpawnPointId.begin(), blockSpawnPointId.end(), SpawnPointId) !=
                blockSpawnPointId.end()) {
                continue; // Bỏ qua các góc lag
            }


            Object *SpawnObjectList = _container->get_Item(ResourceId);
            if (!SpawnObjectList)
                continue;

            String *AssetName = SpawnObjectList->get_field_object<String *>(
                    "<AssetName>k__BackingField");
            if (!AssetName)
                continue;

            std::string AssetNameStr = AssetName->to_string();
            if (CollectSys::GetSpawnType(AssetNameStr) == CollectSys::SpawnType::CardCollect) {
                return objInfo != currentCard ? objInfo : nullptr;
            }
        }
        return nullptr;
    }

    bool TeleportToCard() {
        if (!currentCard || !isValidCard(currentCard)) {
            if (isDebug)
                LOGE("TeleportToCard: Thẻ không hợp lệ");
            return false;
        }
        //        Vector3 pos = currentCard->invoke_method<Vector3>("get_DetectTargetPos");
        //        if (posTarget != Vector3::zero()) {
        //            KinematicCharacterMotor::set_TransientPosition(Vector3(pos.x, pos.y, pos.z));
        //        } else {
        //            KinematicCharacterMotor::set_TransientPosition(Vector3(pos.x, pos.y + 3.0f, pos.z));
        //        }
        //        posTarget = pos;

        Object *tf = currentCard->invoke_method<Object *>("get_transform");
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
        } else {
            KinematicCharacterMotor::set_TransientPosition(Vector3(pos.x, pos.y + 3.0f, pos.z));
        }
        posTarget = pos;
        return true;
    }

    bool isValidCard(Object *card) {
        //        Object *collectSystem = SystemHelper::get_Collect();
        //        if (!collectSystem) {
        //            return false;
        //        }
        //        List<Object*>* list = CollectSystem::_mapObjectInfoList();
        //        if (!list || list->get_Count() < 1) {
        //            return false;
        //        }
        //
        //        if (!list || list->get_Count() < 1) {
        //            return false;
        //        }
        //        for (int i = 0; i < list->get_Count(); i++) {
        //            Object *info = list->get_item(i);
        //            if (!info) {
        //                continue;
        //            }
        //            Object* mapObjectInfo = info->get_field_object<Object*>("MapObjectInfo");
        //            if (!mapObjectInfo) {
        //                continue;
        //            }
        //            uint32_t resourceId = mapObjectInfo->get_field_value<uint32_t>("<ResourceId>k__BackingField");
        //            if (resourceId < 1) {
        //                continue;
        //            }
        //            std::string assetName = TableSpawnObjectListImpl::get_AssetName(resourceId);
        //            if (assetName.empty()) {
        //                continue;
        //            }
        //            if (assetName.find("cardcollect") != std::string::npos) {
        //                if (info == card) {
        //                    return true; // Thẻ hợp lệ
        //                }
        //            }
        //        }
        List<Object *> *mapObjectInfoList = CollectSystem::_mapObjectInfoList();
        if (!mapObjectInfoList) {
            return false;
        }

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
            if (!objInfo) {
                continue;
            }

            Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
            if (!MapObjectInfo) {
                continue;
            }


            int ResourceId = MapObjectInfo->invoke_method<int>("get_ResourceId");
            int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
            uint8_t Step = MapObjectInfo->invoke_method<uint8_t>("get_Step");
            if (Step == 0) {
                continue;
            } // Bỏ qua các mỏ đã thu

            std::vector<int16_t> blockSpawnPointId = {-22231, -22201, -15051, 991, 993, 995, 997,
                    999, 2002, 4873, 30001, 30016, 31107, 31127, 31167, 31203, 21233, 21235, 22001};
            if (std::find(blockSpawnPointId.begin(), blockSpawnPointId.end(), SpawnPointId) !=
                blockSpawnPointId.end()) {
                continue; // Bỏ qua các góc lag
            }


            Object *SpawnObjectList = _container->get_Item(ResourceId);
            if (!SpawnObjectList)
                continue;

            String *AssetName = SpawnObjectList->get_field_object<String *>(
                    "<AssetName>k__BackingField");
            if (!AssetName)
                continue;

            std::string AssetNameStr = AssetName->to_string();
            if (CollectSys::GetSpawnType(AssetNameStr) == CollectSys::SpawnType::CardCollect) {
                if (objInfo == card) {
                    return true; // Mỏ hợp lệ
                }
            }

        }
        return false; // Không tìm thấy thẻ hợp lệ
    }

    Object *FindInsect() {
        auto &cfg = gPLConfig.insect;
        if (!insectList) {
            return nullptr;
        }
        int n = insectList->get_Count();
        if (n <= 0) {
            return nullptr;
        }

        for (int i = 0; i < n; ++i) {
            Object *info = insectList->get_item(i);
            if (!info) {
                continue;
            }
            Object *controller = info->get_field_object<Object *>("control");
            if (!controller) {
                continue;
            }

            if (isBlacklisted((void *) controller)) {
                continue;
            }

            if (!controller->get_field_value<bool>("IsVisible")) {
                if (isDebug)
                    LOGE("FindInsect: Côn trùng không hiển thị, thêm vào blacklist");
                continue;
            }

            int id = controller->invoke_method<int>("get_SubjectID");
            if (id < 1) {
                if (isDebug)
                    LOGE("FindInsect: Côn trùng có ID không hợp lệ, thêm vào blacklist");
                continue;
            }
            if (id == 25054061) {
                if (isDebug)
                    LOGE("FindInsect: bỏ qua quái rơm sự kiện");
                continue;
            }

            std::string assetName = TableItemImpl::GetAssetName(id);
            int grade = TableItemImpl::GetGrade(id);
            bool isHopQua = (assetName.find("box") != std::string::npos ||
                             assetName.find("card") != std::string::npos ||
                             assetName.find("pack") != std::string::npos);
            if (!isHopQua && cfg.minInsectGrade >= 0 && grade <= cfg.minInsectGrade) {
                if (isDebug)
                    LOGE("FindInsect: Côn trùng không đạt yêu cầu, thêm vào blacklist");
                continue;
            }

            std::string name = NguiExtensions::GetNameIDText(id);
            if (name.empty()) {
                name += "HỘP QUÀ/GÓI THẺ";
            }
            //            if (name.empty() && !isHopQua) {
            //                if (isDebug) LOGE("FindInsect: Côn trùng có tên rỗng, thêm vào blacklist");
            //                continue;
            //            }

            {// Kiểm tra bọ an toàn
                if (name.find("(") !=
                    std::string::npos) { // Phát hiện có bọ tên "곤충(Insect) 이름" trong góc lag
                    if (isDebug)
                        LOGE("FindInsect: Phát hiện bọ lạ trong góc lag (%s), thêm vào blacklist",
                             name.c_str());
                    addToBlacklist((void *) controller);
                    continue;
                }
                int _zoneID = controller->get_field_value<int>("_zoneID");
                if (_zoneID < 1001 || _zoneID > 9000) {
                    if (isDebug)
                        LOGE("FindInsect: Phát hiện bọ có _zoneID lạ trong góc lag (%s), thêm vào blacklist",
                             name.c_str());
                    addToBlacklist((void *) controller);
                    continue; // Phát hiện có bọ có _zoneID 9999 trong góc lag
                }
                //                int _senseState = controller->get_field_value<int>("_senseState");
                //                if (_senseState != 0) continue; // Phát hiện có bọ eSenseState Alert và eSenseState Escape trong góc lag
            }

            Vector3 pos = controller->invoke_method<Vector3>("get_DetectTargetPos");


            if (pos.y < 0.01f || pos.y >= cfg.conTrungCachMatDat) {
                continue;
            }

            return controller != currentInsect ? controller : nullptr;
        }
        return nullptr;
    }


    bool TeleportToInsect() {
        if (!currentInsect || !isValidInsect(currentInsect)) {
            if (isDebug)
                LOGE("TeleportToInsect: Côn trùng không hợp lệ");
            return false;
        }

        Vector3 pos = currentInsect->invoke_method<Vector3>("get_DetectTargetPos");

        if (gPLConfig.insect.isBatBoTrenTroi) {
            pos = Vector3(pos.x, 5000, pos.z);
        } else {
            pos = layViTriNgauNhien();
            pos.y += 3.0f;
        }
        KinematicCharacterMotor::set_TransientPosition(pos);
        return true;
    }

    bool isCatchComplete() {
        if (!currentInsect || !isValidInsect(currentInsect)) {
            if (isDebug)
                LOGE("isCatchComplete: Côn trùng không hợp lệ currentInsect=%p, isValidInsect=%d",
                     currentInsect, isValidInsect(currentInsect));
            return false;
        }
        int id = currentInsect->invoke_method<int>("get_SubjectID");
        std::string name = NguiExtensions::GetNameIDText(id);

        //        Object* mainRenderer = currentInsect->get_field_object<Object*>("mainRenderer");
        //        if (!mainRenderer && !name.empty()) {
        //            if (isDebug) LOGE("isCatchComplete: Không tìm thấy mainRenderer của côn trùng");
        //            addToBlacklist((void*)currentInsect);
        //            currentInsect = nullptr;
        //            return false;
        //        }
        //
        //        if (mainRenderer && !mainRenderer->invoke_method<bool>("get_enabled") && !name.empty()) {
        //            if (isDebug) LOGE("isCatchComplete: Côn trùng đã biến mất");
        //            addToBlacklist((void*)currentInsect);
        //            currentInsect = nullptr;
        //            return false;
        //        }

        if (ActorControl::my_Unit) {
            if (ActorControl::my_Unit->get_field_value<bool>("_waterZone")) {
                if (isDebug)
                    LOGE("isCatchComplete: Đang ở trong vùng nước, không thể bắt côn trùng");
                addToBlacklist((void *) currentInsect);
                currentInsect = nullptr;
                return false;
            }
        }

        if (DichChuyenBo()) {
            DialogActionButtons::OnClick();
            catchDistance = InsectSystem::get_distance_catch();
            if (name.empty()) {
                name += "HỘP QUÀ/GÓI THẺ";
            }
            if (isDebug)
                LOGI("isCatchComplete: Bắt Thành Công (%s)", name.c_str());
            addToBlacklist((void *) currentInsect);
            return true;
        } else {
            if (isDebug)
                LOGE("isCatchComplete: Không thể dịch chuyển đến côn trùng");
            addToBlacklist((void *) currentInsect);
            return false;
        }
        return false;
    }

    bool isValidInsect(Object *insect) {
        auto &cfg = gPLConfig.insect;
        if (!insectList) {
            if (isDebug)
                LOGE("isValidInsect: insectList không hợp lệ");
            return false;
        }
        int n = insectList->get_Count();
        if (n <= 0) {
            if (isDebug)
                LOGE("isValidInsect: insectList rỗng");
            return false;
        }


        for (int i = 0; i < n; ++i) {
            Object *info = insectList->get_item(i);
            if (!info) {
                continue;
            }
            Object *controller = info->get_field_object<Object *>("control");
            if (!controller) {
                continue;
            }

            if (controller == insect) {
                if (isBlacklisted((void *) controller)) {
                    if (isDebug)
                        LOGE("isValidInsect: Côn trùng trong blacklist");
                    continue;
                }

                if (!controller->get_field_value<bool>("IsVisible")) {
                    if (isDebug)
                        LOGE("isValidInsect: Côn trùng không hiển thị, thêm vào blacklist");
                    continue;
                }

                int id = controller->invoke_method<int>("get_SubjectID");
                if (id < 1) {
                    if (isDebug)
                        LOGE("isValidInsect: Côn trùng có ID không hợp lệ, thêm vào blacklist");
                    continue;
                }

                std::string assetName = TableItemImpl::GetAssetName(id);
                int grade = TableItemImpl::GetGrade(id);
                bool isHopQua = (assetName.find("box") != std::string::npos ||
                                 assetName.find("card") != std::string::npos ||
                                 assetName.find("pack") != std::string::npos);
                if (!isHopQua && cfg.minInsectGrade >= 0 && grade <= cfg.minInsectGrade) {
                    if (isDebug)
                        LOGE("isValidInsect: Côn trùng không đạt yêu cầu, thêm vào blacklist");
                    continue;
                }

                std::string name = NguiExtensions::GetNameIDText(id);
                if (name.empty()) {
                    name += "HỘP QUÀ/GÓI THẺ";
                }
                //                if (name.empty() && !isHopQua) {
                //                    if (isDebug) LOGE("isValidInsect: Côn trùng có tên rỗng, thêm vào blacklist");
                //                    continue;
                //                }

                {// Kiểm tra bọ an toàn
                    if (name.find("(") !=
                        std::string::npos) { // Phát hiện có bọ tên "곤충(Insect) 이름" trong góc lag
                        if (isDebug)
                            LOGE("isValidInsect: Phát hiện bọ lạ trong góc lag (%s), thêm vào blacklist",
                                 name.c_str());
                        addToBlacklist((void *) controller);
                        continue;
                    }
                    int _zoneID = controller->get_field_value<int>("_zoneID");
                    if (_zoneID < 1001 || _zoneID > 9000) {
                        if (isDebug)
                            LOGE("isValidInsect: Phát hiện bọ có _zoneID lạ trong góc lag (%s), thêm vào blacklist",
                                 name.c_str());
                        continue; // Phát hiện có bọ có _zoneID 9999 trong góc lag
                    }
                    //                    int _senseState = controller->get_field_value<int>("_senseState");
                    //                    if (_senseState != 0) {
                    //                        if (isDebug) LOGE("isValidInsect: Phát hiện bọ có _senseState lạ trong góc lag (%s), thêm vào blacklist", name.c_str());
                    //                        continue; // Phát hiện có bọ eSenseState Alert và eSenseState Escape trong góc lag
                    //                    }
                }

                //                Vector3 pos = controller->invoke_method<Vector3>("get_DetectTargetPos");
                //                if (pos.y < 0.01f || pos.y >= cfg.conTrungCachMatDat) {
                //                    if (isDebug) LOGE("isValidInsect: Côn trùng quá cao so với mặt đất (%s), y=%f", name.c_str(), pos.y);
                //                    continue;
                //                }


                if (isDebug)
                    LOGI("isValidInsect: Côn trùng hợp lệ (%s)", name.c_str());
                return true; // Côn trùng hợp lệ
            }
        }
        if (isDebug)
            LOGE("isValidInsect: Không tìm thấy côn trùng hợp lệ trong danh sách");
        return false; // Không tìm thấy côn trùng hợp lệ
    }


    void set_Position(Vector3 pos) {
        if (!currentInsect) {
            return;
        }
        Object *tf = currentInsect->invoke_method<Object *>("get_transform");
        if (tf) {
            tf->invoke_method<void>("set_position", pos);
        }
    }

    bool DichChuyenBo() {
        if (!currentInsect || !isValidInsect(currentInsect)) {
            currentInsect = nullptr;
            return false;
        }
        Vector3 targetPos = currentInsect->invoke_method<Vector3>("get_DetectTargetPos");
        Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
        Object *myTran = KinematicCharacterMotor::get_transform();
        if (!myTran) {
            return false;
        }
        if (targetPos == Vector3(0, 0, 0) || myPos == Vector3(0, 0, 0)) {
            return false;
        }
        Vector3 forward = myTran->invoke_method<Vector3>("get_forward").Normalize();
        set_Position(myPos + forward * catchDistance);
        if (isDebug)
            LOGI("catchDistance=%f", catchDistance);
        return true;
    }
}

