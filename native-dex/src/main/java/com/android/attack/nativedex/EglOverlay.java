package com.android.attack.nativedex;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

public final class EglOverlay {

    private static final String TAG = "AttackSurface";
    private static final int EGL_ES3 = 0x0040;
    private static final Handler MAIN = new Handler(Looper.getMainLooper());

    private static volatile Activity attachedActivity;
    private static SurfaceView overlay;
    private static RenderThread renderThread;

    private EglOverlay() {}

    static SurfaceView overlayView() {
        return overlay;
    }

    public static void onActivityResumed(Activity activity) {
        if (activity != null) MAIN.post(() -> attach(activity));
    }

    public static void onActivityPaused(Activity activity) {
        if (activity != null) MAIN.post(() -> {
            if (attachedActivity == activity) detach();
        });
    }

    private static void attach(Activity activity) {
        try {
            if (attachedActivity == activity && overlay != null && overlay.getParent() != null) return;
            detach();
            attachedActivity = activity;
            overlay = new SurfaceView(activity);
            overlay.setZOrderOnTop(true);
            overlay.getHolder().setFormat(PixelFormat.TRANSLUCENT);
            renderThread = new RenderThread();
            overlay.getHolder().addCallback(renderThread);
            ViewGroup root = (ViewGroup) activity.getWindow().getDecorView();
            root.addView(overlay, new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT, Gravity.TOP));
            renderThread.start();
            View decor = root;
            decor.setOnApplyWindowInsetsListener((v, insets) -> {
                TouchInputBridge.refreshInsets(activity);
                return insets;
            });
            TouchInputBridge.refreshInsets(activity);
            decor.post(() -> TouchInputBridge.refreshInsets(activity));
            Log.i(TAG, "attached " + activity.getClass().getSimpleName());
        } catch (Throwable t) {
            Log.e(TAG, "attach", t);
            detach();
        }
    }

    private static void detach() {
        RenderThread t = renderThread;
        renderThread = null;
        if (t != null) {
            t.stopRender();
            try {
                t.join(2500L);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        try {
            nativeOnSurfaceDestroyed();
        } catch (Throwable ignored) {}
        if (overlay != null) {
            if (t != null) {
                try {
                    overlay.getHolder().removeCallback(t);
                } catch (Throwable ignored) {}
            }
            ViewGroup p = (ViewGroup) overlay.getParent();
            if (p != null) p.removeView(overlay);
            overlay = null;
        }
        attachedActivity = null;
    }

    private static final class RenderThread extends Thread implements SurfaceHolder.Callback {
        private final Object lock = new Object();
        private volatile boolean running = true;
        private volatile boolean surfaceReady;
        private SurfaceHolder holder;
        private EGLDisplay dpy = EGL14.EGL_NO_DISPLAY;
        private EGLContext ctx = EGL14.EGL_NO_CONTEXT;
        private EGLSurface surf = EGL14.EGL_NO_SURFACE;
        private boolean nativeBound;

        void stopRender() {
            running = false;
            interrupt();
            synchronized (lock) {
                lock.notifyAll();
            }
        }

        @Override
        public void surfaceCreated(SurfaceHolder h) {
            synchronized (lock) {
                holder = h;
                surfaceReady = true;
                lock.notifyAll();
            }
        }

        @Override
        public void surfaceChanged(SurfaceHolder h, int f, int w, int ht) {
            synchronized (lock) {
                holder = h;
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder h) {
            synchronized (lock) {
                surfaceReady = false;
                holder = null;
                unbind();
                releaseEgl();
                lock.notifyAll();
            }
        }

        @Override
        public void run() {
            while (running) {
                SurfaceHolder h;
                synchronized (lock) {
                    while (running && !surfaceReady) {
                        try {
                            lock.wait(100L);
                        } catch (InterruptedException e) {
                            Thread.currentThread().interrupt();
                            return;
                        }
                    }
                    if (!running) return;
                    h = holder;
                }
                if (h == null) continue;
                if (dpy == EGL14.EGL_NO_DISPLAY && !initEgl(h)) {
                    sleepMs(50);
                    continue;
                }
                if (!EGL14.eglMakeCurrent(dpy, surf, surf, ctx)) {
                    sleepMs(16);
                    continue;
                }
                if (!nativeBound) bind(h);
                try {
                    nativeRenderFrame();
                } catch (Throwable t) {
                    Log.e(TAG, "render", t);
                }
                EGL14.eglSwapBuffers(dpy, surf);
                sleepMs(16);
            }
            synchronized (lock) {
                unbind();
                releaseEgl();
            }
        }

        private static boolean surfaceLive(Surface s) {
            return s != null && s.isValid();
        }

        private void bind(SurfaceHolder h) {
            try {
                Surface s = h.getSurface();
                if (surfaceLive(s)) {
                    nativeOnSurfaceCreated(s);
                    nativeBound = true;
                }
            } catch (Throwable t) {
                Log.e(TAG, "bind", t);
            }
        }

        private void unbind() {
            if (!nativeBound) return;
            try {
                nativeOnSurfaceDestroyed();
            } catch (Throwable ignored) {}
            nativeBound = false;
        }

        private boolean initEgl(SurfaceHolder h) {
            dpy = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
            if (dpy == EGL14.EGL_NO_DISPLAY) return false;
            int[] ver = new int[2];
            if (!EGL14.eglInitialize(dpy, ver, 0, ver, 1)) {
                dpy = EGL14.EGL_NO_DISPLAY;
                return false;
            }
            int[] cfgAttr = {
                    EGL14.EGL_RENDERABLE_TYPE, EGL_ES3,
                    EGL14.EGL_RED_SIZE, 8, EGL14.EGL_GREEN_SIZE, 8,
                    EGL14.EGL_BLUE_SIZE, 8, EGL14.EGL_ALPHA_SIZE, 8,
                    EGL14.EGL_DEPTH_SIZE, 16, EGL14.EGL_NONE};
            EGLConfig[] cfg = new EGLConfig[1];
            int[] n = new int[1];
            if (!EGL14.eglChooseConfig(dpy, cfgAttr, 0, cfg, 0, 1, n, 0) || n[0] <= 0) {
                releaseEgl();
                return false;
            }
            Surface win = h.getSurface();
            if (!surfaceLive(win)) {
                releaseEgl();
                return false;
            }
            surf = EGL14.eglCreateWindowSurface(dpy, cfg[0], win, new int[]{EGL14.EGL_NONE}, 0);
            if (surf == null || surf == EGL14.EGL_NO_SURFACE) {
                releaseEgl();
                return false;
            }
            int[] ctxAttr = {EGL14.EGL_CONTEXT_CLIENT_VERSION, 3, EGL14.EGL_NONE};
            ctx = EGL14.eglCreateContext(dpy, cfg[0], EGL14.EGL_NO_CONTEXT, ctxAttr, 0);
            if (ctx == null || ctx == EGL14.EGL_NO_CONTEXT) {
                releaseEgl();
                return false;
            }
            Log.i(TAG, "EGL ES3 ready");
            return true;
        }

        private void releaseEgl() {
            if (dpy != EGL14.EGL_NO_DISPLAY) {
                EGL14.eglMakeCurrent(dpy, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
                if (surf != EGL14.EGL_NO_SURFACE) EGL14.eglDestroySurface(dpy, surf);
                if (ctx != EGL14.EGL_NO_CONTEXT) EGL14.eglDestroyContext(dpy, ctx);
                EGL14.eglTerminate(dpy);
            }
            dpy = EGL14.EGL_NO_DISPLAY;
            ctx = EGL14.EGL_NO_CONTEXT;
            surf = EGL14.EGL_NO_SURFACE;
        }

        private static void sleepMs(long ms) {
            try {
                Thread.sleep(ms);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }

    private static native void nativeOnSurfaceCreated(Surface surface);

    private static native void nativeOnSurfaceDestroyed();

    private static native void nativeRenderFrame();
}
