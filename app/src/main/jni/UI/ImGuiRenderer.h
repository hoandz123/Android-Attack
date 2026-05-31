#ifndef IMGUI_RENDERER_H
#define IMGUI_RENDERER_H

namespace imguiRenderer {

bool startRenderThread();
void requestStop();
void signalFrame();

}

#endif
