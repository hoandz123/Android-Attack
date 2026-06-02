//
// Created by TEAMHMG on 18/07/2025.
//

#pragma once
#ifndef PLAY_PRO_MAX_V2_SCREEN_H
#define PLAY_PRO_MAX_V2_SCREEN_H


#include "Object.h"

namespace UnityEngine {
    struct Screen : public System::Object {
    public:
        static int get_width();
        static int get_height();
        static float get_dpi();
        static void SetResolution(int width, int height, bool fullscreen, int preferredRefreshRate);
    };
}

#endif //PLAY_PRO_MAX_V2_SCREEN_H
