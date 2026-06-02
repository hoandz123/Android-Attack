//
// Created by HMGTEAM on 11/05/2025.
//

#ifndef PLAY_PRO_MAX_CAMERA_H
#define PLAY_PRO_MAX_CAMERA_H

#include "Component.h"
#include "API/Vector3.h"

namespace UnityEngine {
    class Camera : public UnityEngine::Component {
    public:
        //Method
        static Camera* get_main();
        static Camera* get_current();

        Vector3 WorldToScreenPoint(Vector3 position);

        static Vector3 StaticWorldToScreenPoint(Vector3 position) {
            Camera* camera = get_main();
            if (!camera) {
                camera = get_current();
                if (!camera) {
                    return Vector3();
                }
            }
            return camera->WorldToScreenPoint(position);
        }
    };
}



#endif //PLAY_PRO_MAX_CAMERA_H
