//
// Created by TEAMHMG on 12/04/2026.
//

/*
 * ┌──────────────────────┬──────────────────────────────────────────────┐
 * │ State                │ Mô tả                                       │
 * ├──────────────────────┼──────────────────────────────────────────────┤
 * │ None                 │ Tắt / chờ bật                                │
 * │ FindTreasure         │ Tìm rương Hide gần nhất trong danh sách     │
 * │ MovingToTreasure     │ Di chuyển zigzag sóng sin bằng joystick     │
 * │ Digging              │ Giả lập nhấn Action liên tục để đào         │
 * │ BuyShovel            │ Mở shop + mua xẻng khi hết                  │
 * │ RandomWalk           │ Di ngẫu nhiên khi không còn rương            │
 * │ Resting              │ Nghỉ ngẫu nhiên 2-5s giữa các rương         │
 * └──────────────────────┴──────────────────────────────────────────────┘
 *
 * Zigzag sóng sin 10 cấp độ:
 *   amplitude  = 0.05 + (cấp - 1) * 0.083    →  0.05 .. 0.80
 *   frequency  = 1.0  + (cấp - 1) * 0.22     →  1.0  .. 3.0
 *   speedMul   = 1.0  - (cấp - 1) * 0.07     →  1.0  .. 0.37
 */

#include "AutoTreasure.h"
#include "Config/Config.h"
#include "KinematicCharacterMotor.h"
#include "DialogJoyStick.h"
#include "DialogShopInGame.h"
#include "SystemHelper.h"
#include "enum/TreasureBox_State.h"
#include "enum/eDialogType.h"
#include "UnityEngine/Camera.h"
#include "UnityEngine/Transform.h"
#include "Tools/Tools.h"
#include "PlayLog.h"
#include <GameUI/EspGUI.h>
#include <GameUI/GameViewport.h>
#include <imgui.h>

namespace AutoTreasure {

    eAutoTreasure curState = eAutoTreasure::None;
    std::vector<TreasureInfo> treasureList;
    TreasureInfo currentTarget = {};
    Object *myActor = nullptr;

    static float elapsedMoveTime = 0.0f;
    static long long lastUpdateMs = 0;
    static long long stateEnterMs = 0;
    static long long lastActionPressMs = 0;
    static long long randomWalkChangeMs = 0;
    static Vector2 randomWalkDir = Vector2::Zero();
    static int restDurationMs = 0;
    static long long lastNearbyCheckMs = 0;
    static bool isPausedNearPlayer = false;
    static float smoothAngle = 0.0f;
    static Vector3 startPosition = Vector3::zero();

    static Vector3 lastStuckCheckPos = Vector3::zero();
    static long long lastStuckCheckMs = 0;
    static int stuckCount = 0;
    static int unstuckAttempt = 0;
    static float unstuckAngle = 0.0f;
    static float unstuckDistWalked = 0.0f;
    static Vector3 unstuckStartPos = Vector3::zero();
    static int skippedTargetUid = -1;

    static std::mt19937 rng(std::random_device{}());

    float RandomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    int RandomInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }

    void OnTreasureScan(Object *instance, Vector3 position, int uid, int boxType) {
        for (auto &t : treasureList) {
            if (t.uid == uid) {
                t.position = position;
                t.instance = instance;
                t.boxType = boxType;
                return;
            }
        }
        treasureList.push_back({instance, position, uid, boxType});
    }

    void ClearTreasureList() {
        treasureList.clear();
    }

    void RemoveTreasure(int uid) {
        for (auto it = treasureList.begin(); it != treasureList.end(); ++it) {
            if (it->uid == uid) {
                treasureList.erase(it);
                return;
            }
        }
    }

    void Reset() {
        curState = eAutoTreasure::None;
        currentTarget = {};
        myActor = nullptr;
        elapsedMoveTime = 0.0f;
        lastUpdateMs = 0;
        stateEnterMs = 0;
        lastActionPressMs = 0;
        isPausedNearPlayer = false;
        stuckCount = 0;
        unstuckAttempt = 0;
        skippedTargetUid = -1;
        lastStuckCheckMs = 0;
        DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
    }

    static void ChangeState(eAutoTreasure newState) {
        curState = newState;
        if (newState == eAutoTreasure::MovingToTreasure) {
            stuckCount = 0;
            lastStuckCheckMs = 0;
        }
        stateEnterMs = Tools::getSystemMilliseconds();
        elapsedMoveTime = 0.0f;
        smoothAngle = 0.0f;
    }

    static float GetDistanceXZ(Vector3 a, Vector3 b) {
        float dx = a.x - b.x;
        float dz = a.z - b.z;
        return sqrtf(dx * dx + dz * dz);
    }

    static TreasureInfo *FindNearestTreasure(Vector3 myPos) {
        auto &cfg = gPLConfig.miniGame.digging;
        TreasureInfo *best = nullptr;
        float bestDist = cfg.maxKhoangCach;

        for (auto &t : treasureList) {
            if (!t.instance) continue;
            if (t.uid == skippedTargetUid) continue;

            int64_t ownerUID = t.instance->invoke_method<int64_t>("get_OwnerUID");
            if (ownerUID == 0) continue;

            auto state = t.instance->invoke_method<TreasureBox_State>("get_CurrentTreasureState");
            if (state != TreasureBox_State::Hide) continue;

            int type = t.boxType;
            if (!cfg.filterLoaiRuong.empty()) {
                auto it = cfg.filterLoaiRuong.find(type);
                if (it != cfg.filterLoaiRuong.end() && !it->second) continue;
            }

            float dist = GetDistanceXZ(myPos, t.position);
            if (dist < 10.0f) continue;
            if (dist > cfg.radiusTim) continue;

            if (dist < bestDist) {
                bestDist = dist;
                best = &t;
            }
        }
        return best;
    }

    /*
     * Joystick input trong Unity tương đối với camera:
     *   joystick.y > 0 → nhân vật đi theo camera forward (chiếu xuống XZ)
     *   joystick.x > 0 → nhân vật đi theo camera right (chiếu xuống XZ)
     *
     * Để di chuyển theo hướng world (worldDirX, worldDirZ):
     *   joystick.x = dot(worldDir, camRight_XZ)
     *   joystick.y = dot(worldDir, camForward_XZ)
     */
    static Vector2 WorldDirToJoystick(float worldDirX, float worldDirZ) {
        UnityEngine::Camera *cam = UnityEngine::Camera::get_main();
        if (!cam || !cam->isValid()) return Vector2(worldDirX, worldDirZ);

        UnityEngine::Transform *camTrans = cam->get_transform();
        if (!camTrans || !camTrans->isValid()) return Vector2(worldDirX, worldDirZ);

        Vector3 camFwd = camTrans->get_forward();
        Vector3 camRgt = camTrans->get_right();

        float fwdLen = sqrtf(camFwd.x * camFwd.x + camFwd.z * camFwd.z);
        float rgtLen = sqrtf(camRgt.x * camRgt.x + camRgt.z * camRgt.z);
        if (fwdLen < 0.001f || rgtLen < 0.001f) return Vector2(worldDirX, worldDirZ);

        float camFwdX = camFwd.x / fwdLen;
        float camFwdZ = camFwd.z / fwdLen;
        float camRgtX = camRgt.x / rgtLen;
        float camRgtZ = camRgt.z / rgtLen;

        float joyX = worldDirX * camRgtX + worldDirZ * camRgtZ;
        float joyY = worldDirX * camFwdX + worldDirZ * camFwdZ;

        return Vector2(joyX, joyY);
    }

    /*
     * Đường chữ S thực trong không gian:
     *
     * 1. Lưu startPos khi bắt đầu di chuyển
     * 2. Trục chính = đường thẳng startPos → targetPos
     * 3. Trục vuông góc = perpendicular trên mặt XZ
     * 4. Tính tiến trình t (0→1) dọc trục chính
     * 5. Waypoint = điểm trên trục + sin(t * sốChuKỳ * 2π) * doRongChuS
     * 6. Joystick hướng về waypoint phía trước
     *
     * ┌──────────┬──────────────┬───────────────┐
     * │ Cấp      │ Số chu kỳ S  │ Độ rộng (m)   │
     * ├──────────┼──────────────┼───────────────┤
     * │ 1        │ 3            │ 2m            │
     * │ 5        │ 7            │ 4m            │
     * │ 10       │ 12           │ 6m            │
     * └──────────┴──────────────┴───────────────┘
     */
    static constexpr float STRAIGHT_DIST = 3.0f;

    static Vector2 CalcZigzagJoystick(Vector3 myPos, Vector3 targetPos, float dt) {
        auto &cfg = gPLConfig.miniGame.digging;
        float distToTarget = GetDistanceXZ(myPos, targetPos);

        if (distToTarget < STRAIGHT_DIST) {
            float toX = targetPos.x - myPos.x;
            float toZ = targetPos.z - myPos.z;
            if (distToTarget < 0.01f) return Vector2::Zero();
            float dirX = toX / distToTarget;
            float dirZ = toZ / distToTarget;
            float mag = (distToTarget < 1.5f) ? 0.7f : 1.0f;
            Vector2 joy = WorldDirToJoystick(dirX * mag, dirZ * mag);
            float m = joy.Magnitude();
            if (m > 1.0f) joy = joy / m;
            return joy;
        }

        int cấp = std::max(1, std::min(10, cfg.capDoAnToan));
        float sốChuKỳ = 3.0f + (cấp - 1) * 1.0f;
        float doRong = 2.0f + (cấp - 1) * 0.44f;

        if (cfg.chuKyChuS > 0.5f) sốChuKỳ = cfg.chuKyChuS;
        if (cfg.doCongChuS > 0.1f) doRong = cfg.doCongChuS;

        float axisX = targetPos.x - startPosition.x;
        float axisZ = targetPos.z - startPosition.z;
        float totalDist = sqrtf(axisX * axisX + axisZ * axisZ);
        if (totalDist < 1.0f) totalDist = 1.0f;

        float axisNX = axisX / totalDist;
        float axisNZ = axisZ / totalDist;
        float perpX = -axisNZ;
        float perpZ = axisNX;

        float toMeX = myPos.x - startPosition.x;
        float toMeZ = myPos.z - startPosition.z;
        float traveled = toMeX * axisNX + toMeZ * axisNZ;
        if (traveled < 0.0f) traveled = 0.0f;

        float safeDist = totalDist - STRAIGHT_DIST;
        if (safeDist < 1.0f) safeDist = 1.0f;

        float lookAhead = 2.5f;
        float aheadPos = traveled + lookAhead;
        if (aheadPos > safeDist) aheadPos = safeDist;

        float tAhead = aheadPos / safeDist;
        if (tAhead > 1.0f) tAhead = 1.0f;

        float fade = 1.0f;
        float fadeStart = safeDist * 0.85f;
        if (traveled > fadeStart) {
            fade = 1.0f - (traveled - fadeStart) / (safeDist - fadeStart);
            if (fade < 0.0f) fade = 0.0f;
        }

        float offset = sinf(tAhead * sốChuKỳ * 2.0f * (float)M_PI) * doRong * fade;

        float waypointX = startPosition.x + axisNX * aheadPos + perpX * offset;
        float waypointZ = startPosition.z + axisNZ * aheadPos + perpZ * offset;

        float toWpX = waypointX - myPos.x;
        float toWpZ = waypointZ - myPos.z;
        float toWpDist = sqrtf(toWpX * toWpX + toWpZ * toWpZ);
        if (toWpDist < 0.01f) return Vector2::Zero();

        Vector2 joystick = WorldDirToJoystick(toWpX / toWpDist, toWpZ / toWpDist);
        float mag = joystick.Magnitude();
        if (mag > 1.0f) joystick = joystick / mag;

        return joystick;
    }

    static Vector2 CalcRandomWalkJoystick() {
        long long now = Tools::getSystemMilliseconds();
        if (randomWalkChangeMs == 0 || now - randomWalkChangeMs > RandomInt(2000, 5000)) {
            float angle = RandomFloat(0.0f, 2.0f * (float)M_PI);
            float worldX = cosf(angle);
            float worldZ = sinf(angle);
            randomWalkDir = WorldDirToJoystick(worldX, worldZ);
            randomWalkChangeMs = now;
        }
        float speed = RandomFloat(0.3f, 0.7f);
        return randomWalkDir * speed;
    }

    void Update(Object *actorInstance) {
        auto &cfg = gPLConfig.miniGame.digging;
        if (!cfg.isAutoDigTreasure || !cfg.isEnable || isGameLoading) {
            if (curState != eAutoTreasure::None) {
                Reset();
            }
            return;
        }

        myActor = actorInstance;
        if (!myActor) return;


        long long now = Tools::getSystemMilliseconds();
        float dt = 0.016f;
        if (lastUpdateMs > 0) {
            dt = (float)(now - lastUpdateMs) / 1000.0f;
            dt = std::min(dt, 0.1f);
        }
        lastUpdateMs = now;

        Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();

        if (curState == eAutoTreasure::None) {
            ChangeState(eAutoTreasure::FindTreasure);
            return;
        }

        switch (curState) {

        case eAutoTreasure::FindTreasure: {
            if (now - stateEnterMs < 500) return;

            TreasureInfo *nearest = FindNearestTreasure(myPos);
            if (nearest) {
                currentTarget = *nearest;
                currentTarget.ownerUID = nearest->instance->invoke_method<int64_t>("get_OwnerUID");
                startPosition = myPos;
                LOGD("AutoTreasure: Tìm thấy rương UID=%d, owner=%ld, khoảng cách=%.1f", currentTarget.uid, (long)currentTarget.ownerUID, GetDistanceXZ(myPos, currentTarget.position));
                ChangeState(eAutoTreasure::MovingToTreasure);
            } else {
                if (cfg.autoMoveKhiHetRuong) {
                    ChangeState(eAutoTreasure::RandomWalk);
                }
            }
            break;
        }

        case eAutoTreasure::MovingToTreasure: {
            if (!currentTarget.instance) {
                ChangeState(eAutoTreasure::FindTreasure);
                return;
            }

            int64_t nowOwner = currentTarget.instance->invoke_method<int64_t>("get_OwnerUID");
            if (nowOwner == 0) {
                LOGD("AutoTreasure: Rương UID=%d mất chủ khi đang đi", currentTarget.uid);
                DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                RemoveTreasure(currentTarget.uid);
                ChangeState(eAutoTreasure::FindTreasure);
                return;
            }

            auto state = currentTarget.instance->invoke_method<TreasureBox_State>("get_CurrentTreasureState");
            if (state != TreasureBox_State::Hide) {
                LOGD("AutoTreasure: Rương UID=%d đã đổi state, tìm rương khác", currentTarget.uid);
                DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                RemoveTreasure(currentTarget.uid);
                ChangeState(eAutoTreasure::FindTreasure);
                return;
            }

            UnityEngine::Transform *transform = currentTarget.instance->invoke_method<UnityEngine::Transform *>("get_transform");
            if (transform && transform->isValid()) {
                currentTarget.position = transform->get_position();
            }

            float dist = GetDistanceXZ(myPos, currentTarget.position);

            if (dist < 0.3f) {
                DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                LOGD("AutoTreasure: Đã tới rương UID=%d, bắt đầu đào", currentTarget.uid);
                ChangeState(eAutoTreasure::Digging);
                return;
            }

            if (lastStuckCheckMs == 0) {
                lastStuckCheckMs = now;
                lastStuckCheckPos = myPos;
            } else if (now - lastStuckCheckMs > 500) {
                float moved = GetDistanceXZ(myPos, lastStuckCheckPos);
                if (moved < 0.15f) {
                    stuckCount++;
                    LOGD("AutoTreasure: Có thể bị kẹt (lần %d), di chuyển %.2f m trong 0.5s", stuckCount, moved);
                    if (stuckCount >= 3) {
                        LOGD("AutoTreasure: Bị kẹt! Thử tránh vật cản");
                        DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                        skippedTargetUid = currentTarget.uid;
                        unstuckAttempt = 0;
                        unstuckAngle = RandomFloat(0.0f, 2.0f * (float)M_PI);
                        unstuckDistWalked = 0.0f;
                        unstuckStartPos = myPos;
                        ChangeState(eAutoTreasure::Unstuck);
                        return;
                    }
                } else {
                    stuckCount = 0;
                }
                lastStuckCheckMs = now;
                lastStuckCheckPos = myPos;
            }

            Vector2 joystick = CalcZigzagJoystick(myPos, currentTarget.position, dt);
            DialogJoyStick::OnMoveJoyStickEvent(joystick);
            break;
        }

        case eAutoTreasure::Digging: {
            if (!currentTarget.instance) {
                ChangeState(eAutoTreasure::FindTreasure);
                return;
            }

            auto state = currentTarget.instance->invoke_method<TreasureBox_State>("get_CurrentTreasureState");

            if (state == TreasureBox_State::Complete) {
                static long long completeTimeMs = 0;
                if (completeTimeMs == 0) {
                    completeTimeMs = now;
                    LOGD("AutoTreasure: Rương UID=%d đào xong, chờ nhận quà...", currentTarget.uid);
                } else if (now - completeTimeMs > 500) {
                    Object *headUp = currentTarget.instance->get_field_object<Object *>("headUpBoxOpen");
                    if (headUp) {
                        headUp->invoke_method<void>("OnClick_BoxOpen");
                        LOGD("AutoTreasure: Đã nhấn nhận quà rương UID=%d", currentTarget.uid);
                    }
                    completeTimeMs = 0;
                    RemoveTreasure(currentTarget.uid);
                    restDurationMs = RandomInt(2000, 5000);
                    ChangeState(eAutoTreasure::Resting);
                }
                return;
            }

            if (state == TreasureBox_State::RewardOpen ||
                state == TreasureBox_State::Fade || state == TreasureBox_State::End) {
                LOGD("AutoTreasure: Rương UID=%d state=%d, chuyển rương tiếp", currentTarget.uid, (int)state);
                RemoveTreasure(currentTarget.uid);
                restDurationMs = RandomInt(2000, 5000);
                ChangeState(eAutoTreasure::Resting);
                return;
            }

            int sốXẻng = myActor->invoke_method<int>("get_EquipGameItemCount");
            if (sốXẻng <= 0) {
                LOGD("AutoTreasure: Hết xẻng, chuyển sang mua");
                ChangeState(eAutoTreasure::BuyShovel);
                return;
            }

            int delayAction = RandomInt(50, 300);
            if (now - lastActionPressMs > delayAction) {
                DialogJoyStick::OnPress_ActionButton();
                lastActionPressMs = now;
            }
            break;
        }

        case eAutoTreasure::BuyShovel: {
            static int buyStep = 0;

            if (buyStep == 0) {
                Object *dialogSys = SystemHelper::get_Dialog();
                if (dialogSys) {
                    static int shopType = (int)eDialogType::ShopInGame;
                    static int cmdCreate = 1;
                    dialogSys->invoke_method<void>("Command", shopType, cmdCreate, (Object *)nullptr);
                    LOGD("AutoTreasure: Mở DialogShopInGame qua DialogSystem");
                } else {
                    LOGE("AutoTreasure: DialogSystem NULL");
                    ChangeState(eAutoTreasure::FindTreasure);
                    return;
                }
                buyStep = 1;
                stateEnterMs = now;
                return;
            }

            if (buyStep == 1 && now - stateEnterMs > 1000) {
                Object *shop = DialogShopInGame::get_Instance();
                if (shop) {
                    static bool valTrue = true;
                    shop->invoke_method<void>("set_EquipRefreshAll", valTrue);
                    shop->invoke_method<void>("OnClick_ItemBuy1");
                    LOGD("AutoTreasure: Đã nhấn mua xẻng");
                    buyStep = 2;
                    stateEnterMs = now;
                } else {
                    if (now - stateEnterMs > 5000) {
                        LOGE("AutoTreasure: Shop không mở được (timeout)");
                        buyStep = 0;
                        ChangeState(eAutoTreasure::FindTreasure);
                    }
                }
                return;
            }

            if (buyStep == 2 && now - stateEnterMs > 1000) {
                int sốXẻng = myActor->invoke_method<int>("get_EquipGameItemCount");
                Object *shop = DialogShopInGame::get_Instance();
                if (shop) {
                    shop->invoke_method<void>("DialogCloseFromBack");
                }
                buyStep = 0;
                if (sốXẻng > 0) {
                    LOGD("AutoTreasure: Mua xẻng thành công, có %d xẻng", sốXẻng);
                    ChangeState(eAutoTreasure::Digging);
                } else {
                    LOGD("AutoTreasure: Mua xẻng thất bại");
                    ChangeState(eAutoTreasure::FindTreasure);
                }
            }
            break;
        }

        case eAutoTreasure::RandomWalk: {
            if (now - stateEnterMs > 2000) {
                TreasureInfo *nearest = FindNearestTreasure(myPos);
                if (nearest) {
                    DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                    currentTarget = *nearest;
                    ChangeState(eAutoTreasure::MovingToTreasure);
                    return;
                }
                stateEnterMs = now;
            }

            Vector2 joystick = CalcRandomWalkJoystick();
            DialogJoyStick::OnMoveJoyStickEvent(joystick);
            break;
        }

        case eAutoTreasure::Resting: {
            DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
            if (now - stateEnterMs > restDurationMs) {
                ChangeState(eAutoTreasure::FindTreasure);
            }
            break;
        }

        case eAutoTreasure::Unstuck: {
            /*
             * Tránh vật cản:
             * 1. Đi theo unstuckAngle khoảng 10m
             * 2. Mỗi 0.5s check có bị kẹt tiếp không
             * 3. Nếu kẹt tiếp → xoay 90° thử hướng khác (tối đa 4 lần)
             * 4. Sau khi thoát → tìm rương khác (bỏ qua rương cũ bị kẹt)
             */
            unstuckDistWalked = GetDistanceXZ(myPos, unstuckStartPos);

            if (unstuckDistWalked >= 10.0f) {
                LOGD("AutoTreasure: Đã tránh vật cản thành công (%.1fm), tìm rương mới", unstuckDistWalked);
                DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                ChangeState(eAutoTreasure::FindTreasure);
                return;
            }

            if (lastStuckCheckMs == 0) {
                lastStuckCheckMs = now;
                lastStuckCheckPos = myPos;
            } else if (now - lastStuckCheckMs > 500) {
                float moved = GetDistanceXZ(myPos, lastStuckCheckPos);
                if (moved < 0.15f) {
                    unstuckAttempt++;
                    if (unstuckAttempt >= 4) {
                        LOGD("AutoTreasure: Không thể tránh vật cản sau 4 lần, bỏ qua rương UID=%d", skippedTargetUid);
                        DialogJoyStick::OnMoveJoyStickEvent(Vector2::Zero());
                        ChangeState(eAutoTreasure::FindTreasure);
                        return;
                    }
                    unstuckAngle += (float)M_PI * 0.5f;
                    unstuckStartPos = myPos;
                    unstuckDistWalked = 0.0f;
                    LOGD("AutoTreasure: Vẫn kẹt, thử hướng khác (lần %d)", unstuckAttempt);
                }
                lastStuckCheckMs = now;
                lastStuckCheckPos = myPos;
            }

            float dirX = cosf(unstuckAngle);
            float dirZ = sinf(unstuckAngle);
            Vector2 joystick = WorldDirToJoystick(dirX, dirZ);
            float mag = joystick.Magnitude();
            if (mag > 1.0f) joystick = joystick / mag;
            DialogJoyStick::OnMoveJoyStickEvent(joystick);
            break;
        }

        default:
            ChangeState(eAutoTreasure::FindTreasure);
            break;
        }
    }

    void DrawESP() {
        if (curState == eAutoTreasure::None) return;
        if (!currentTarget.instance) return;
        auto &cfg = gPLConfig.miniGame.digging;
        if (!cfg.isEnable || !cfg.isAutoDigTreasure) return;
        UnityEngine::Camera *camera = UnityEngine::Camera::get_main();
        if (!camera || !camera->isValid()) return;
        Vector3 playerPos = KinematicCharacterMotor::get_TransientPosition();
        Vector3 playerScreen = camera->WorldToScreenPoint(playerPos);
        Vector3 targetScreen = camera->WorldToScreenPoint(currentTarget.position);
        if (playerScreen.z <= 0 && targetScreen.z <= 0) return;
        int glWidth = GameViewport::width();
        int glHeight = GameViewport::height();
        float scaleX = (float) ImGui::GetIO().DisplaySize.x / (float) glWidth;
        float scaleY = (float) ImGui::GetIO().DisplaySize.y / (float) glHeight;
        ImVec2 pScreen(playerScreen.x * scaleX, ImGui::GetIO().DisplaySize.y - (playerScreen.y * scaleY));
        ImVec2 tScreen(targetScreen.x * scaleX, ImGui::GetIO().DisplaySize.y - (targetScreen.y * scaleY));
        ImVec4 lineColor;
        const char *stateText = "";
        switch (curState) {
            case eAutoTreasure::FindTreasure: lineColor = ImVec4(1.f, 1.f, 0.3f, 0.8f); stateText = OBF("Finding"); break;
            case eAutoTreasure::MovingToTreasure: lineColor = ImVec4(0.3f, 1.f, 0.3f, 0.9f); stateText = OBF("Moving"); break;
            case eAutoTreasure::Digging: lineColor = ImVec4(1.f, 0.5f, 0.f, 0.9f); stateText = OBF("Digging"); break;
            case eAutoTreasure::BuyShovel: lineColor = ImVec4(0.3f, 0.7f, 1.f, 0.8f); stateText = OBF("BuyShovel"); break;
            case eAutoTreasure::RandomWalk: lineColor = ImVec4(0.7f, 0.7f, 0.7f, 0.6f); stateText = OBF("RandomWalk"); break;
            case eAutoTreasure::Resting: lineColor = ImVec4(0.5f, 0.5f, 1.f, 0.6f); stateText = OBF("Resting"); break;
            case eAutoTreasure::Unstuck: lineColor = ImVec4(1.f, 0.2f, 0.2f, 0.9f); stateText = OBF("Unstuck"); break;
            default: lineColor = ImVec4(1.f, 1.f, 1.f, 0.5f); stateText = ""; break;
        }
        if (targetScreen.z > 0) {
            if (curState == eAutoTreasure::MovingToTreasure || curState == eAutoTreasure::Digging || curState == eAutoTreasure::Unstuck) EspGUI::DrawLine(pScreen, tScreen, 2.f, lineColor);
            float dist = GetDistanceXZ(playerPos, currentTarget.position);
            char label[64];
            snprintf(label, sizeof(label), OBF("[%s] %.1fm"), stateText, dist);
            EspGUI::DrawTooltip(tScreen, label);
        }
    }

}
