package com.android.attack.nativedex;

import android.app.Activity;
import android.os.Build;
import android.os.Handler;
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

    private static final String TAG = "ATTACK_Touch";
    private static final Handler MAIN = new Handler(Looper.getMainLooper());

    private static volatile Activity attachedActivity;
    private static Window.Callback previousCallback;
    private static Object proxyCallback;
    private static int touchDispatchCount;
    private static int touchFedCount;
    /** Reuse on main thread — feedTouch chỉ chạy trên UI thread qua Window.Callback. */
    private static final int[] sTouchLoc = new int[2];

    private TouchInputBridge() {}

    public static void install(Activity activity) {
        if (activity == null) return;
        // setAttributes/setCallback đụng view hierarchy → bắt buộc main thread.
        if (Looper.myLooper() != Looper.getMainLooper()) {
            MAIN.post(() -> install(activity));
            return;
        }
        try {
            if (attachedActivity == activity && proxyCallback != null) {
                ensureCallback(activity);
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
            wrapWindowCallback(activity, w, base);
            refreshInsets(activity);
            Log.i(TAG, "install ok (main) " + activity.getClass().getSimpleName()
                    + " delegate=" + base.getClass().getSimpleName());
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

    /** Gắn lại proxy; nếu app/engine ghi đè callback thì bọc delegate mới (không chồng proxy). */
    private static void ensureCallback(Activity activity) {
        if (activity == null || proxyCallback == null) return;
        try {
            Window w = activity.getWindow();
            if (w == null) return;
            Window.Callback cur = w.getCallback();
            if (cur == proxyCallback) return;
            Window.Callback base = cur != null ? cur : activity;
            if (base == proxyCallback) return;
            Log.w(TAG, "callback replaced, re-wrap " + activity.getClass().getSimpleName());
            wrapWindowCallback(activity, w, base);
        } catch (Throwable t) {
            Log.w(TAG, "ensureCallback", t);
        }
    }

    private static void wrapWindowCallback(Activity activity, Window w, Window.Callback base) {
        previousCallback = base;
        final Window.Callback delegate = base;
        InvocationHandler handler = (proxy, method, args) -> {
            if ("dispatchTouchEvent".equals(method.getName()) && args != null && args.length == 1
                    && args[0] instanceof MotionEvent) {
                feedTouch((MotionEvent) args[0]);
                touchDispatchCount++;
            } else if ("dispatchKeyEvent".equals(method.getName()) && args != null && args.length == 1
                    && args[0] instanceof KeyEvent) {
                KeyboardInputBridge.feedKeyEvent((KeyEvent) args[0]);
            }
            return method.invoke(delegate, args);
        };
        proxyCallback = Proxy.newProxyInstance(
                base.getClass().getClassLoader(), new Class<?>[] {Window.Callback.class}, handler);
        w.setCallback((Window.Callback) proxyCallback);
    }

    private static View touchCoordView() {
        SurfaceView ov = EglOverlay.overlayView();
        if (ov != null && ov.getParent() != null) return ov;
        Activity a = attachedActivity;
        if (a == null) return null;
        return a.getWindow() != null ? a.getWindow().getDecorView() : null;
    }

    private static void feedTouch(MotionEvent event) {
        View ref = touchCoordView();
        if (ref == null) {
            if (touchDispatchCount <= 3 || touchDispatchCount % 50 == 0) {
                Log.w(TAG, "feedTouch skip: no coord view (dispatch #" + touchDispatchCount + ")");
            }
            return;
        }
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
                if (touchDispatchCount <= 3) {
                    Log.d(TAG, "feedTouch skip: action=" + masked);
                }
                return;
        }
        touchFedCount++;
        int ptr = masked == MotionEvent.ACTION_MOVE ? 0 : event.getActionIndex();
        ref.getLocationOnScreen(sTouchLoc);
        float x;
        float y;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            x = event.getRawX(ptr) - sTouchLoc[0];
            y = event.getRawY(ptr) - sTouchLoc[1];
        } else {
            x = event.getRawX() - sTouchLoc[0];
            y = event.getRawY() - sTouchLoc[1];
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
