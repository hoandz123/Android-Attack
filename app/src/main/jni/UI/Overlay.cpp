#include "UI/Overlay.h"
#include "UI/ImGuiRenderer.h"
#include "UI/OverlaySurface.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"

namespace overlay {

bool start() {
    try {
        if (!overlaySurface::create()) { LOGE(OBFUSCATE("overlay::start: create failed")); return false; }
        if (!overlaySurface::waitReady(10000)) { LOGE(OBFUSCATE("overlay::start: surface not ready")); return false; }
        if (!imguiRenderer::startRenderThread()) { LOGE(OBFUSCATE("overlay::start: render thread failed")); return false; }
        LOGI(OBFUSCATE("overlay::start: ok"));
        return true;
    } catch (...) {
        LOGE(OBFUSCATE("overlay::start: exception"));
        return false;
    }
}

}
