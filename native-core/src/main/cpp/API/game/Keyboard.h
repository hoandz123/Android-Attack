#pragma once

#include <functional>
enum class TouchScreenKeyboardStatus {
    Visible = 0,
    Done = 1,
    Canceled = 2,
    LostFocus = 3
};

namespace Keyboard {
    extern bool check;
    void Open(const char *text, const std::function<void(const std::string &)> &callback);
    void Reset();
    void Update();
}; // namespace Keyboard
