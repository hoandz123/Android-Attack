#include "PlayLog.h"
#include "CollectSystem.h"
#include <API/Il2CppApi.h>
#include "SystemHelper.h"
#include "Config/Config.h"
#include "TableSystem.h"
#include "UnityEngine/Camera.h"

#include <unordered_set>
#include "Stubs/AutoCollect.h"
#include "Stubs/ESPManager.h"
#include "ActorControl.h"
#include "Stubs/AutoMonster.h"
#include "Stubs/AutoFarm.h"

namespace CollectSystem {
    Class *get_class() {
        return FindClass("CollectSystem");
    }
    Object *get_Instance() {
        return SystemHelper::get_Collect();
    }
    List<Object *> *_mapObjectInfoList() {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->get_field_object<List<Object *> *>("_mapObjectInfoList");
    }

    List<Object *> *_fieldMonsterInfoList() {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->get_field_object<List<Object *> *>("_fieldMonsterInfoList");
    }

    void Update() {
        auto &collect = gPLConfig.collect;
        auto &monster = gPLConfig.monster;
        if (!(collect.isAutoDapDa || collect.isAutoNhatThe || collect.isAutoNguyenLieu || collect.esp.isEnable)) return;
        if (!ActorControl::my_Motor) return;

        if (collect.esp.isEnable) {
            List<Object*> *mapObjectInfoList = _mapObjectInfoList();
            if (!mapObjectInfoList) return;

            static Object *TItem = TableSystem::GetTableUnit<Object *>(FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("SpawnObjectList"));
            if (!TItem) {
                LOGE("CollectSystem::Update - !TItem");
                TItem = TableSystem::GetTableUnit<Object *>(FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("SpawnObjectList"));
                return;
            }

            static Dictionary<int, Object*> *_container = TItem->get_field_object<Dictionary<int, Object*>*>("_container");
            if (!_container) {
                LOGE("CollectSystem::Update - !_container");
                _container = TItem->get_field_object<Dictionary<int, Object*>*>("_container");
                return;
            }

            UnityEngine::Camera* cam = UnityEngine::Camera::get_main();
            if (!cam) return;

            // Block list cache sẵn
            static const std::unordered_set<int> blockSpawnPointId = {
                    -22231, -22201, -15051, 991, 993, 995, 997, 999,
                    2002, 4873, 30001, 30016, 31107, 31127, 31167,
                    31203, 21233, 21235, 22001
            };

            int count = mapObjectInfoList->get_Count();
            for (int i = 0; i < count; i++) {
                Object *objInfo = mapObjectInfoList->get_item(i);
                if (!objInfo) continue;

                Object *MapObjectInfo = objInfo->get_field_object<Object *>("MapObjectInfo");
                if (!MapObjectInfo) continue;

                int ResourceId   = MapObjectInfo->invoke_method<int>("get_ResourceId");
                int SpawnPointId = MapObjectInfo->invoke_method<int>("get_SpawnPointId");
                if (blockSpawnPointId.count(SpawnPointId)) continue;

                Object *SpawnObjectList = _container->get_Item(ResourceId);
                if (!SpawnObjectList) continue;

                String *AssetName = SpawnObjectList->get_field_object<String *>("<AssetName>k__BackingField");
                if (!AssetName) continue;

                std::string assetStr = AssetName->to_string();
                CollectSys::SpawnType type = CollectSys::GetSpawnType(assetStr);
                if (type == CollectSys::SpawnType::Unknown) continue;

                // Kiểm tra có bật filter cho loại này không
                bool show = (
                        (collect.esp.isCardCollect && type == CollectSys::SpawnType::CardCollect) ||
                        (collect.esp.isCoin && type == CollectSys::SpawnType::Coin) ||
                        (collect.esp.isDragonVillageMonster && type == CollectSys::SpawnType::DragonVillageMonster) ||
                        (collect.esp.isFishBreadShop && type == CollectSys::SpawnType::FishBreadShop) ||
                        (collect.esp.isFishingZone && type == CollectSys::SpawnType::FishingZone) ||
                        (collect.esp.isFossil && type == CollectSys::SpawnType::Fossil) ||
                        (collect.esp.isGathering && type == CollectSys::SpawnType::Gathering) ||
                        (collect.esp.isIng && type == CollectSys::SpawnType::Ing) ||
                        (collect.esp.isNameTag && type == CollectSys::SpawnType::NameTag) ||
                        (collect.esp.isOre && type == CollectSys::SpawnType::Ore) ||
                        (collect.esp.isPlants && type == CollectSys::SpawnType::Plants) ||
                        (collect.esp.isSlime && type == CollectSys::SpawnType::Slime) ||
                        (collect.esp.isSnowman && type == CollectSys::SpawnType::Snowman) ||
                        (collect.esp.isVein && (type == CollectSys::SpawnType::Vein || type == CollectSys::SpawnType::GemVein))
                );
                if (!show) continue;

                Vector3 pos = objInfo->invoke_method<Vector3>("get_DetectTargetPos");
                Vector3 screenPos = cam->WorldToScreenPoint(pos);

                std::string name;
                if (collect.esp.isShowName) {
                    name = CollectSys::GetSpawnTypeName(type);
                    name += "\nID: " + std::to_string(ResourceId);
                    name += "\nindex: " + std::to_string(i);
                }

                ESPManager::Add(objInfo, (collect.esp.isTeleportButton ? pos : Vector3()), screenPos, name);
            }
        }

        if (monster.esp.isEnable) {
            List<Object*> *fieldMonsterInfoList = _fieldMonsterInfoList();
            if (!fieldMonsterInfoList) return;
            static Object *TItem = TableSystem::GetTableUnit<Object *>(FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("SpawnMonsterList"));
            if (!TItem) {
                LOGE("CollectSystem::Update - !TItem");
                TItem = TableSystem::GetTableUnit<Object *>(FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("SpawnMonsterList"));
                return;
            }

            static Dictionary<int, Object*> *_container = TItem->get_field_object<Dictionary<int, Object*>*>("_container");
            if (!_container) {
                LOGE("CollectSystem::Update - !_container");
                _container = TItem->get_field_object<Dictionary<int, Object*>*>("_container");
                return;
            }

            UnityEngine::Camera* cam = UnityEngine::Camera::get_main();
            if (!cam) return;

            // Block list cache sẵn
            static const std::unordered_set<int> blockSpawnPointId = {
                    -22231, -22201, -15051, 991, 993, 995, 997, 999,
                    2002, 4873, 30001, 30016, 31107, 31127, 31167,
                    31203, 21233, 21235, 22001
            };

            int count = fieldMonsterInfoList->get_Count();
            for (int i = 0; i < count; i++) {
                Object *objInfo = fieldMonsterInfoList->get_item(i);
                if (!objInfo) continue;

                Object *MonsterInfo = objInfo->get_field_object<Object *>("MonsterInfo");
                if (!MonsterInfo) continue;

                int ResourceId   = MonsterInfo->invoke_method<int>("get_ResourceId");
                int SpawnPointId = MonsterInfo->invoke_method<int>("get_SpawnPointId");
                int ObjectHp     = MonsterInfo->invoke_method<int>("get_ObjectHp");
                int ObjectMaxHp  = MonsterInfo->invoke_method<int>("get_ObjectMaxHp");
                if (ObjectHp <= 0 || ObjectMaxHp <= 0) continue; // Chỉ hiện thị quái còn sống
                if (blockSpawnPointId.count(SpawnPointId)) continue;

                Object *SpawnMonsterList = _container->get_Item(ResourceId);
                if (!SpawnMonsterList) continue;

                String *AssetName = SpawnMonsterList->get_field_object<String *>("<AssetName>k__BackingField");
                if (!AssetName) continue;

                std::string assetStr = AssetName->to_string();

                Vector3 pos = MonsterInfo->invoke_method<Vector3>("get_Position");
                Vector3 screenPos = cam->WorldToScreenPoint(pos);

                std::string name;
                if (monster.esp.isShowName) {
                    name += "HP: " + std::to_string(ObjectHp) + "/" + std::to_string(ObjectMaxHp);
                    name += "\nID: " + std::to_string(ResourceId);
                }

                ESPManager::Add(objInfo, (monster.esp.isTeleportButton ? pos : Vector3()), screenPos, name, ObjectMaxHp > 50000 ? ImVec4(1, 0, 0, 1) : ImVec4(1,1,1,1));
            }
        }

        if (monster.isEnable && monster.isAutoMonster) {
            Monster::Update();
        }

        if (collect.isAutoDapDa || collect.isAutoNhatThe || collect.isAutoNguyenLieu) {
            CollectSys::Update();
        }
        
    }
}
