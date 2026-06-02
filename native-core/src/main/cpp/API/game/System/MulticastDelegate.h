//
// Created by TEAMHMG on 05/09/2025.
//

#pragma once
#ifndef PLAY_IL2CPP_MULTICASTDELEGATE_H
#define PLAY_IL2CPP_MULTICASTDELEGATE_H

#include "Delegate.h"

namespace System {
    class MulticastDelegate : public System::Object {
    public:
        Array<Delegate *> delegates;

    };
}


#endif //PLAY_IL2CPP_MULTICASTDELEGATE_H
