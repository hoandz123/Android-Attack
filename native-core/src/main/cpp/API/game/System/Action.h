//
// Created by TEAMHMG on 15/08/2025.
//

#pragma once
#ifndef PLAY_ACTION_H
#define PLAY_ACTION_H

#include "MulticastDelegate.h"

namespace System {
    template<typename... Args>
    class ActionT : public System::MulticastDelegate {
    public:
        void Invoke(Args... args){
            constexpr std::size_t count = sizeof...(Args);
            invoke_method<void>(count, "Invoke", to_voidptr(std::forward<Args>(args))...);
        }
    };
    class Action : public System::MulticastDelegate {
    public:
        void Invoke(){
            invoke_method<void>("Invoke");
        }
    };
}



#endif //PLAY_ACTION_H
