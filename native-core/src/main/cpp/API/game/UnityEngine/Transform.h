//
// Created by HMGTEAM on 11/05/2025.
//

#ifndef PLAY_PRO_MAX_TRANSFORM_H
#define PLAY_PRO_MAX_TRANSFORM_H
#include "API/Vector3.h"
#include "API/Quaternion.h"
#include "Component.h"

namespace UnityEngine {
    class Transform : public Component {
    public:
        //Method
        Vector3 get_position();
        void set_position(Vector3 position);

        Vector3 get_localPosition();
        void set_localPosition(Vector3 position);

        Quaternion get_rotation();
        void set_rotation(Quaternion rotation);

        Vector3 get_forward();
        void set_forward(Vector3 forward);
        Vector3 get_right();
        void set_right(Vector3 right);
        Vector3 get_up();
        void set_up(Vector3 up);
        Vector3 get_localScale();
        void set_localScale(Vector3 scale);
    };
}

#endif //PLAY_PRO_MAX_TRANSFORM_H
