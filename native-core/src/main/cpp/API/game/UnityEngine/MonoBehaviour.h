//
// Created by HMGTEAM on 11/05/2025.
//

#ifndef PLAY_PRO_MAX_MONOBEHAVIOUR_H
#define PLAY_PRO_MAX_MONOBEHAVIOUR_H

#include "Component.h"

namespace UnityEngine {
    class MonoBehaviour : public UnityEngine::Component {
    public:
        void* m_CancellationTokenSource;
    };
}


#endif //PLAY_PRO_MAX_MONOBEHAVIOUR_H
