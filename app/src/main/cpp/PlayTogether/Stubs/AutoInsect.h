//
// Created by TEAMHMG on 08/09/2025.
//

#pragma once
#ifndef PLAY_IL2CPP_AUTOINSECT_H
#define PLAY_IL2CPP_AUTOINSECT_H

#include <unordered_set>
#include <unwind.h>
#include <dlfcn.h>
#include "API/Il2CppApi.h"


namespace InsectSys {
    extern bool isStopDiTrenKhong;
    extern bool isDebug;
    enum class eAutoState {
        None,
        FindingCard,
        TeleportingCard,
        WaitingForCard,
        FindingInsect,
        Teleporting,
        WaitingForTeleport,
        CatchingInsect,
        WaitingForCatch,
        NextMap,
        WaitingForNextMap,
        SellInsect,
    };

    enum class eSellState {
        None,
        CheckMap,
        NextMapSell,
        WaitingForNextMap,
        CheckPosition,
        SellInsect,
        SellComplete,
    };


    extern std::unordered_set<void *> blacklist;

    void addToBlacklist(void *ptr);
    bool isBlacklisted(void *ptr);

    extern List<Object *> *insectList;
    extern float catchDistance;
    extern eAutoState state;
    extern eSellState sellState;
    extern Object *currentInsect;
    extern Object *currentCard;
    extern int targetMapId;
    extern Vector3 posTarget;


    void set_Position(Vector3 pos);
    bool DichChuyenBo();
    Object *FindInsect();
    bool TeleportToInsect();
    bool isCatchComplete();
    bool isValidInsect(Object *insect);

    Object *FindCard();
    bool TeleportToCard();
    bool isValidCard(Object *card);

    void Update(List<Object *> *_liveInsectList);
}

#endif //PLAY_IL2CPP_AUTOINSECT_H
