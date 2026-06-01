package com.android.attack.nativedex;

import android.app.Activity;
import android.os.Build;
import android.os.Looper;
import android.util.Log;
import android.view.DisplayCutout;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Proxy;

/** Window.Callback proxy: feed ImGui touch, always forward to delegate. */
public final class TouchInputBridge {

    private static final String TAG = "AttackTouch";

    private static volatile Activity attachedActivity;
    private static Window.Callback previousCallback;
    private static Object proxyCallback;

    private TouchInputBridge() {}

    public static void install(Activity activity) {
        if (activity == null) return;
        try {
            if (attachedActivity == activity && proxyCallback != null) {
                refreshInsets(activity);
                return;
            }
            if (attachedActivity != null) uninstall(attachedActivity);
            attachedActivity = activity;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                Window w = activity.getWindow();
                WindowManager.LayoutParams lp = w.getAttributes();
                lp.layoutInDisplayCutoutMode =
                        WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
                w.setAttributes(lp);
            }
            Window w = activity.getWindow();
            Window.Callback base = w.getCallback();
            if (base == null) base = activity;
            if (proxyCallback != null && base == proxyCallback) {
                refreshInsets(activity);
                return;
            }
            previousCallback = base;
            final Window.Callback delegate = base;
            InvocationHandler handler = (proxy, method, args) -> {
                if ("dispatchTouchEvent".equals(method.getName()) && args != null && args.length == 1
                        && args[0] instanceof MotionEvent) {
                    feedTouch((MotionEvent) args[0]);
                } else if ("dispatchKeyEvent".equals(method.getName()) && args != null && args.length == 1
                        && args[0] instanceof KeyEvent) {
                    KeyboardInputBridge.feedKeyEvent((KeyEvent) args[0]);
                }
                return method.invoke(delegate, args);
            };
            proxyCallback = Proxy.newProxyInstance(
                    base.getClass().getClassLoader(), new Class<?>[] {Window.Callback.class}, handler);
            w.setCallback((Window.Callback) proxyCallback);
            refreshInsets(activity);
            Log.i(TAG, "install ok");
        } catch (Throwable t) {
            Log.e(TAG, "install", t);
            uninstall(activity);
        }
    }

    /** Chỉ gỡ proxy của {@code activity} đang giữ — tránh race resume(Main) trước pause(Second). */
    public static void uninstall(Activity activity) {
        if (activity == null || attachedActivity != activity) return;
        try {
            if (proxyCallback != null) {
                Window w = activity.getWindow();
                if (w != null && w.getCallback() == proxyCallback && previousCallback != null) {
                    w.setCallback(previousCallback);
                }
            }
        } catch (Throwable t) {
            Log.w(TAG, "uninstall", t);
        }
        attachedActivity = null;
        previousCallback = null;
        proxyCallback = null;
    }

    private static void feedTouch(MotionEvent event) {
        SurfaceView ov = EglOverlay.overlayView();
        if (ov == null || ov.getParent() == null) return;
        int masked = event.getActionMasked();
        int action;
        switch (masked) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                action = 0;
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                action = 1;
                break;
            case MotionEvent.ACTION_MOVE:
                action = 2;
                break;
            case MotionEvent.ACTION_CANCEL:
                action = 3;
                break;
            default:
                return;
        }
        int ptr = masked == MotionEvent.ACTION_MOVE ? 0 : event.getActionIndex();
        int[] loc = new int[2];
        ov.getLocationOnScreen(loc);
        float x;
        float y;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            x = event.getRawX(ptr) - loc[0];
            y = event.getRawY(ptr) - loc[1];
        } else {
            x = event.getRawX() - loc[0];
            y = event.getRawY() - loc[1];
        }
        nativeOnTouch(action, x, y);
    }

    static void refreshInsets(Activity activity) {
        if (activity == null) return;
        try {
            View decor = activity.getWindow().getDecorView();
            if (decor == null) return;
            int l = 0, t = 0, r = 0, b = 0;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                WindowInsets ins = decor.getRootWindowInsets();
                if (ins != null) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                        DisplayCutout c = ins.getDisplayCutout();
                        if (c != null) {
                            l = Math.max(l, c.getSafeInsetLeft());
                            t = Math.max(t, c.getSafeInsetTop());
                            r = Math.max(r, c.getSafeInsetRight());
                            b = Math.max(b, c.getSafeInsetBottom());
                        }
                    }
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                        android.graphics.Insets sys = ins.getInsets(android.view.WindowInsets.Type.systemBars());
                        l = Math.max(l, sys.left);
                        t = Math.max(t, sys.top);
                        r = Math.max(r, sys.right);
                        b = Math.max(b, sys.bottom);
                    } else {
                        l = Math.max(l, ins.getSystemWindowInsetLeft());
                        t = Math.max(t, ins.getSystemWindowInsetTop());
                        r = Math.max(r, ins.getSystemWindowInsetRight());
                        b = Math.max(b, ins.getSystemWindowInsetBottom());
                    }
                }
            }
            final int fl = l, ft = t, fr = r, fb = b;
            final float density = activity.getResources().getDisplayMetrics().density;
            Runnable apply = () -> {
                nativeUpdateInsets(fl, ft, fr, fb);
                nativeUpdateDisplayMetrics(density);
            };
            if (Looper.myLooper() == Looper.getMainLooper()) apply.run();
            else decor.post(apply);
        } catch (Throwable t) {
            Log.w(TAG, "refreshInsets", t);
        }
    }

    private static native void nativeOnTouch(int action, float x, float y);

    private static native void nativeUpdateInsets(int left, int top, int right, int bottom);

    private static native void nativeUpdateDisplayMetrics(float density);
}
