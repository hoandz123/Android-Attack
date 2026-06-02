#include "DrawRender.hpp"
#include "ModUi.hpp"

void DrawRender::registerTask(void (*task)()) {
    modui::RegisterOverlayDraw(task);
}
