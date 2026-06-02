//
// Created by HMGTEAM on 23/05/2025.
//

#ifndef PLAY_PRO_MAX_APPLICATION_H
#define PLAY_PRO_MAX_APPLICATION_H
#include "iostream"
#include "Object.h"


class Application : public UnityEngine::Object {
public:
    static void OpenURL(std::string url);
    static int get_targetFrameRate();
    static void set_targetFrameRate(int value);
};


#endif //PLAY_PRO_MAX_APPLICATION_H
