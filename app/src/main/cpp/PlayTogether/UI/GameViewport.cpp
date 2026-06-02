#include "GameViewport.h"
#include <UnityEngine/Screen.h>
#include <imgui.h>

namespace GameViewport {

int width() {
    int w = UnityEngine::Screen::get_width();
    if (w > 0) return w;
    return (int) ImGui::GetIO().DisplaySize.x;
}

int height() {
    int h = UnityEngine::Screen::get_height();
    if (h > 0) return h;
    return (int) ImGui::GetIO().DisplaySize.y;
}

}
