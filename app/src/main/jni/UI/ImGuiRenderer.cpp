#include "UI/ImGuiRenderer.h"
#include "UI/Menu.h"
#include "UI/OverlaySurface.h"
#include "UI/TouchInput.h"
#include "Core/Bootstrap.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace imguiRenderer {

namespace {

std::atomic<bool> g_running{false};
std::atomic<bool> g_stop{false};
std::thread g_renderThread;
std::mutex g_frameMutex;
std::condition_variable g_frameCv;
std::atomic<bool> g_framePending{false};
EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
EGLSurface g_eglSurface = EGL_NO_SURFACE;
EGLContext g_eglContext = EGL_NO_CONTEXT;
bool g_useEs3 = true;
int g_msaaSamples = 0;
const char *g_glslVersion = OBFUSCATE("#version 300 es");

bool tryChooseEglConfig(EGLDisplay display, bool wantEs3, int samples, EGLConfig *outConfig) {
    if (samples > 0) {
        const EGLint attribsEs3Msaa[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, samples, EGL_NONE};
        const EGLint attribsEs2Msaa[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, samples, EGL_NONE};
        EGLint numConfigs = 0;
        if (wantEs3 && eglChooseConfig(display, attribsEs3Msaa, outConfig, 1, &numConfigs) == EGL_TRUE && numConfigs > 0) return true;
        numConfigs = 0;
        if (eglChooseConfig(display, attribsEs2Msaa, outConfig, 1, &numConfigs) == EGL_TRUE && numConfigs > 0) return true;
        return false;
    }
    const EGLint attribsEs3[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, EGL_NONE};
    const EGLint attribsEs2[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, EGL_NONE};
    EGLint numConfigs = 0;
    if (wantEs3 && eglChooseConfig(display, attribsEs3, outConfig, 1, &numConfigs) == EGL_TRUE && numConfigs > 0) return true;
    numConfigs = 0;
    if (eglChooseConfig(display, attribsEs2, outConfig, 1, &numConfigs) == EGL_TRUE && numConfigs > 0) return true;
    return false;
}

bool chooseEglConfig(EGLDisplay display, EGLConfig *outConfig, bool *outEs3) {
    const int sampleList[] = {4, 2, 0};
    for (int i = 0; i < 3; i++) {
        if (tryChooseEglConfig(display, true, sampleList[i], outConfig)) { *outEs3 = true; g_msaaSamples = sampleList[i]; return true; }
    }
    for (int i = 0; i < 3; i++) {
        if (tryChooseEglConfig(display, false, sampleList[i], outConfig)) { *outEs3 = false; g_msaaSamples = sampleList[i]; return true; }
    }
    return false;
}

bool initEgl(ANativeWindow *window) {
    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY) { LOGE(OBFUSCATE("imguiRenderer: eglGetDisplay failed")); return false; }
    if (eglInitialize(g_eglDisplay, nullptr, nullptr) != EGL_TRUE) { LOGE(OBFUSCATE("imguiRenderer: eglInitialize failed")); return false; }
    EGLConfig eglConfig = nullptr;
    if (!chooseEglConfig(g_eglDisplay, &eglConfig, &g_useEs3)) { LOGE(OBFUSCATE("imguiRenderer: eglChooseConfig failed")); return false; }
    EGLint nativeVisualId = 0;
    eglGetConfigAttrib(g_eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &nativeVisualId);
    ANativeWindow_setBuffersGeometry(window, 0, 0, nativeVisualId);
    const EGLint ctxAttribsEs3[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    const EGLint ctxAttribsEs2[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    g_eglContext = eglCreateContext(g_eglDisplay, eglConfig, EGL_NO_CONTEXT, g_useEs3 ? ctxAttribsEs3 : ctxAttribsEs2);
    if (g_eglContext == EGL_NO_CONTEXT) { LOGE(OBFUSCATE("imguiRenderer: eglCreateContext failed")); return false; }
    g_eglSurface = eglCreateWindowSurface(g_eglDisplay, eglConfig, window, nullptr);
    if (g_eglSurface == EGL_NO_SURFACE) { LOGE(OBFUSCATE("imguiRenderer: eglCreateWindowSurface failed")); return false; }
    if (eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext) != EGL_TRUE) { LOGE(OBFUSCATE("imguiRenderer: eglMakeCurrent failed")); return false; }
    g_glslVersion = g_useEs3 ? OBFUSCATE("#version 300 es") : OBFUSCATE("#version 100");
    if (g_msaaSamples > 0) { (void)g_msaaSamples; }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    eglSwapInterval(g_eglDisplay, 0);
    return true;
}

void shutdownEgl() {
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_eglContext != EGL_NO_CONTEXT) eglDestroyContext(g_eglDisplay, g_eglContext);
        if (g_eglSurface != EGL_NO_SURFACE) eglDestroySurface(g_eglDisplay, g_eglSurface);
        eglTerminate(g_eglDisplay);
    }
    g_eglDisplay = EGL_NO_DISPLAY;
    g_eglSurface = EGL_NO_SURFACE;
    g_eglContext = EGL_NO_CONTEXT;
}

void renderLoop() {
    bootstrap::getEnv();
    ANativeWindow *window = overlaySurface::getWindow();
    if (!window) { LOGE(OBFUSCATE("imguiRenderer: no ANativeWindow")); return; }
    if (!initEgl(window)) { LOGE(OBFUSCATE("imguiRenderer: initEgl failed")); return; }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.ConfigInputTextCursorBlink = true;
    overlayMenu::setupFonts();
    overlayMenu::initTheme();
    if (!ImGui_ImplOpenGL3_Init(g_glslVersion)) { LOGE(OBFUSCATE("imguiRenderer: ImGui_ImplOpenGL3_Init failed")); ImGui::DestroyContext(); shutdownEgl(); return; }
    overlayMenu::onGlRendererReady();
    LOGI(OBFUSCATE("imguiRenderer: started (%s) msaa=%d"), g_glslVersion, g_msaaSamples);
    auto frameClockStart = std::chrono::steady_clock::now();
    while (!g_stop.load()) {
        {
            std::unique_lock<std::mutex> lock(g_frameMutex);
            g_frameCv.wait_for(lock, std::chrono::milliseconds(250), []() { return g_framePending.load() || g_stop.load(); });
            if (g_stop.load()) break;
            g_framePending.store(false);
        }
        window = overlaySurface::getWindow();
        if (window == nullptr) continue;
        EGLint eglWidth = 0;
        EGLint eglHeight = 0;
        eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_WIDTH, &eglWidth);
        eglQuerySurface(g_eglDisplay, g_eglSurface, EGL_HEIGHT, &eglHeight);
        int width = (eglWidth > 0) ? (int)eglWidth : overlaySurface::getWidth();
        int height = (eglHeight > 0) ? (int)eglHeight : overlaySurface::getHeight();
        if (width <= 0 || height <= 0) continue;
        ImGui_ImplOpenGL3_NewFrame();
        ImGuiIO &ioFrame = ImGui::GetIO();
        ioFrame.DisplaySize = ImVec2((float)width, (float)height);
        ioFrame.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        auto frameClockNow = std::chrono::steady_clock::now();
        float deltaSeconds = std::chrono::duration<float>(frameClockNow - frameClockStart).count();
        frameClockStart = frameClockNow;
        if (deltaSeconds <= 0.0f) deltaSeconds = 1.0f / 45.0f;
        ioFrame.DeltaTime = deltaSeconds;
        touchInput::applyPendingTouch();
        ImGui::NewFrame();
        overlayMenu::draw(width, height);
        touchInput::syncInputWindowAfterFrame();
        ImGui::Render();
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(g_eglDisplay, g_eglSurface);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    shutdownEgl();
    LOGI(OBFUSCATE("imguiRenderer: stopped"));
}

}

bool startRenderThread() {
    if (g_running.exchange(true)) return true;
    g_stop.store(false);
    g_renderThread = std::thread([]() {
        try { renderLoop(); } catch (...) { LOGE(OBFUSCATE("imguiRenderer: renderLoop exception")); }
        g_running.store(false);
    });
    g_renderThread.detach();
    return true;
}

void requestStop() {
    g_stop.store(true);
    g_frameCv.notify_all();
}

void signalFrame() {
    g_framePending.store(true);
    g_frameCv.notify_one();
}

}
