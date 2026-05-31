package com.android.attack;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

public final class NativeBridge {
    public static final int MAX_FPS = 45;
    private static final long FRAME_INTERVAL_MS = 1000L / MAX_FPS;
    private NativeBridge() {
    }

    private static SurfaceView sOverlayView;
    private static View sInputView;
    private static WindowManager sWindowManager;
    private static WindowManager.LayoutParams sInputLp;
    private static android.os.Handler sFrameHandler;
    private static final Runnable sFrameTick = new Runnable() {
        @Override
        public void run() {
            nativeSignalFrame();
            if (sFrameHandler != null) sFrameHandler.postDelayed(this, FRAME_INTERVAL_MS);
        }
    };

    static native void nativeRun(long id);
    static native void nativeOnError(long id, String trace);
    static native boolean nativeOnTouch(long id, int action, float rawX, float rawY);
    static native void nativeOnSurfaceReady(Surface surface, int width, int height);
    static native void nativeOnSurfaceChanged(Surface surface, int width, int height);
    static native void nativeOnSurfaceDestroyed();
    static native void nativeSignalFrame();

    private static void startFrameLoop(Activity activity) {
        stopFrameLoop();
        sFrameHandler = new android.os.Handler(activity.getMainLooper());
        sFrameHandler.postDelayed(sFrameTick, FRAME_INTERVAL_MS);
    }

    private static void stopFrameLoop() {
        if (sFrameHandler != null) {
            sFrameHandler.removeCallbacks(sFrameTick);
            sFrameHandler = null;
        }
    }

    public static void createOverlay(Activity activity) {
        if (activity == null) return;
        if (sOverlayView != null) return;
        WindowManager wm = (WindowManager) activity.getSystemService("window");
        if (wm == null) return;
        sWindowManager = wm;
        android.os.IBinder token = activity.getWindow().getDecorView().getWindowToken();
        SurfaceView sv = new SurfaceView(activity);
        sv.setZOrderOnTop(true);
        sv.getHolder().setFormat(PixelFormat.TRANSLUCENT);
        sv.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Surface surface = holder.getSurface();
                nativeOnSurfaceReady(surface, holder.getSurfaceFrame().width(), holder.getSurfaceFrame().height());
                startFrameLoop(activity);
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                nativeOnSurfaceChanged(holder.getSurface(), width, height);
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                stopFrameLoop();
                nativeOnSurfaceDestroyed();
            }
        });
        int panelType = WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;
        int renderFlags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;
        WindowManager.LayoutParams renderLp = new WindowManager.LayoutParams(WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT, panelType, renderFlags, PixelFormat.TRANSLUCENT);
        renderLp.gravity = Gravity.TOP | Gravity.LEFT;
        renderLp.token = token;
        wm.addView(sv, renderLp);
        sOverlayView = sv;
        View inputView = new View(activity);
        inputView.setBackgroundColor(0);
        int inputFlags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        WindowManager.LayoutParams inputLp = new WindowManager.LayoutParams(0, 0, panelType, inputFlags, PixelFormat.TRANSLUCENT);
        inputLp.gravity = Gravity.TOP | Gravity.LEFT;
        inputLp.token = token;
        inputView.setVisibility(View.GONE);
        inputView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                return nativeOnTouch(0L, event.getActionMasked(), event.getRawX(), event.getRawY());
            }
        });
        wm.addView(inputView, inputLp);
        sInputView = inputView;
        sInputLp = inputLp;
    }

    public static void updateInputRect(int x, int y, int w, int h) {
        if (sInputView == null || sWindowManager == null || sInputLp == null) return;
        if (w <= 0 || h <= 0) {
            hideInputRect();
            return;
        }
        sInputLp.x = x;
        sInputLp.y = y;
        sInputLp.width = w;
        sInputLp.height = h;
        sInputView.setVisibility(View.VISIBLE);
        sWindowManager.updateViewLayout(sInputView, sInputLp);
    }

    public static void hideInputRect() {
        if (sInputView == null || sWindowManager == null || sInputLp == null) return;
        sInputLp.x = 0;
        sInputLp.y = 0;
        sInputLp.width = 0;
        sInputLp.height = 0;
        sInputView.setVisibility(View.GONE);
        sWindowManager.updateViewLayout(sInputView, sInputLp);
    }

    static final class Run implements Runnable {
        private final long id;

        Run(long id) {
            this.id = id;
        }

        @Override
        public void run() {
            try {
                nativeRun(id);
            } catch (Throwable t) {
                nativeOnError(id, android.util.Log.getStackTraceString(t));
            }
        }
    }

}
