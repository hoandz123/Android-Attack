package com.android.attack.nativedex;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.ArrayMap;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

public final class ActivityTrackerBridge implements Application.ActivityLifecycleCallbacks {

    private static final String TAG = "ATTACK_ActivityTracker";
    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    private static final long PROBE_INTERVAL_MS = 200L;
    private static final int PROBE_MAX_ATTEMPTS = 200;

    private static volatile boolean installed;
    private static ActivityTrackerBridge instance;
    private static volatile boolean overlayProbeRunning;
    private static int overlayProbeAttempts;

    public static boolean install() {
        if (installed) {
            syncExistingActivities();
            startOverlayProbe();
            return true;
        }
        synchronized (ActivityTrackerBridge.class) {
            if (installed) {
                syncExistingActivities();
                startOverlayProbe();
                return true;
            }
            try {
                Application app = currentApplication();
                if (app == null) {
                    Log.e(TAG, "install: no Application");
                    return false;
                }
                instance = new ActivityTrackerBridge();
                app.registerActivityLifecycleCallbacks(instance);
                installed = true;
                Log.i(TAG, "install ok");
                syncExistingActivities();
                startOverlayProbe();
                return true;
            } catch (Throwable t) {
                Log.e(TAG, "install", t);
                return false;
            }
        }
    }

    private static Application currentApplication() {
        try {
            for (int i = 0; i < 30; i++) {
                Class<?> at = Class.forName("android.app.ActivityThread");
                Method m = at.getDeclaredMethod("currentApplication");
                m.setAccessible(true);
                Application app = (Application) m.invoke(null);
                if (app != null) return app;
                Thread.sleep(20L);
            }
        } catch (Throwable t) {
            Log.e(TAG, "currentApplication", t);
        }
        return null;
    }

    /** Quét ActivityThread.mActivities — không phụ thuộc lifecycle callback. */
    private static List<Activity> collectResumedActivities() {
        List<Activity> out = new ArrayList<>();
        try {
            Class<?> at = Class.forName("android.app.ActivityThread");
            Method m = at.getDeclaredMethod("currentActivityThread");
            m.setAccessible(true);
            Object thread = m.invoke(null);
            if (thread == null) return out;
            Field f = at.getDeclaredField("mActivities");
            f.setAccessible(true);
            if (!(f.get(thread) instanceof ArrayMap)) return out;
            ArrayMap<?, ?> map = (ArrayMap<?, ?>) f.get(thread);
            for (Object rec : map.values()) {
                if (rec == null) continue;
                Field paused = rec.getClass().getDeclaredField("paused");
                paused.setAccessible(true);
                if (paused.getBoolean(rec)) continue;
                Field actF = rec.getClass().getDeclaredField("activity");
                actF.setAccessible(true);
                Object a = actF.get(rec);
                if (a instanceof Activity) {
                    Activity act = (Activity) a;
                    if (!act.isFinishing() && !act.isDestroyed()) out.add(act);
                }
            }
        } catch (Throwable t) {
            Log.w(TAG, "collectResumed", t);
        }
        return out;
    }

    private static Activity pickOverlayTarget(List<Activity> activities) {
        Activity fallback = null;
        for (Activity a : activities) {
            if (a == null) continue;
            String n = a.getClass().getName();
            if (n.contains("Unity") || n.contains("MessagingUnityPlayer")) return a;
            if (fallback == null) fallback = a;
        }
        return fallback;
    }

    private static boolean canAttachWindow(Activity activity) {
        if (activity == null) return false;
        try {
            if (activity.getWindow() == null) return false;
            return activity.getWindow().getDecorView() != null;
        } catch (Throwable t) {
            return false;
        }
    }

    /** Gắn overlay nếu lifecycle/sync bỏ lỡ activity đang resumed. */
    private static void ensureOverlayAttached() {
        if (EglOverlay.isAttached()) return;
        List<Activity> resumed = collectResumedActivities();
        Activity target = pickOverlayTarget(resumed);
        if (target == null || !canAttachWindow(target)) return;
        Log.i(TAG, "probe attach " + target.getClass().getSimpleName());
        dispatchResumed(target);
    }

    private static void startOverlayProbe() {
        if (!installed || overlayProbeRunning) return;
        overlayProbeRunning = true;
        overlayProbeAttempts = 0;
        MAIN.post(overlayProbeRunnable);
    }

    private static final Runnable overlayProbeRunnable = new Runnable() {
        @Override
        public void run() {
            if (!installed) {
                overlayProbeRunning = false;
                return;
            }
            ensureOverlayAttached();
            overlayProbeAttempts++;
            if (EglOverlay.isAttached()) {
                overlayProbeRunning = false;
                Log.i(TAG, "overlay probe ok");
                return;
            }
            if (overlayProbeAttempts >= PROBE_MAX_ATTEMPTS) {
                overlayProbeRunning = false;
                Log.w(TAG, "overlay probe timeout");
                return;
            }
            MAIN.postDelayed(this, PROBE_INTERVAL_MS);
        }
    };

    private static void syncExistingActivities() {
        List<Activity> resumed = collectResumedActivities();
        int n = 0;
        for (Activity a : resumed) {
            dispatchResumed(a);
            n++;
        }
        if (n > 0) Log.i(TAG, "synced " + n + " activities");
    }

    private static void dispatchResumed(Activity a) {
        if (a == null) return;
        try {
            Log.i(TAG, "resumed " + a.getClass().getName());
            nativeOnResumed(a);
            if (Looper.myLooper() == Looper.getMainLooper()) {
                attachViews(a);
            } else {
                a.runOnUiThread(() -> attachViews(a));
            }
        } catch (Throwable t) {
            Log.e(TAG, "resume", t);
        }
    }

    private static void attachViews(Activity a) {
        EglOverlay.onActivityResumed(a);
        TouchInputBridge.install(a);
        KeyboardInputBridge.install(a);
        TouchInputBridge.refreshInsets(a);
    }

    private static void dispatchPaused(Activity a) {
        if (a == null) return;
        try {
            Log.i(TAG, "paused " + a.getClass().getName());
            EglOverlay.onActivityPaused(a);
            TouchInputBridge.uninstall(a);
            KeyboardInputBridge.uninstall(a);
            nativeOnPaused(a);
        } catch (Throwable t) {
            Log.e(TAG, "pause", t);
        }
    }

    @Override
    public void onActivityResumed(Activity activity) {
        dispatchResumed(activity);
        if (!EglOverlay.isAttached()) startOverlayProbe();
    }

    @Override
    public void onActivityPaused(Activity activity) {
        dispatchPaused(activity);
    }

    @Override
    public void onActivityDestroyed(Activity activity) {
        dispatchPaused(activity);
        if (activity != null) {
            try {
                nativeOnDestroyed(activity);
            } catch (Throwable t) {
                Log.e(TAG, "destroy", t);
            }
        }
    }

    @Override
    public void onActivityCreated(Activity activity, Bundle savedInstanceState) {}

    @Override
    public void onActivityStarted(Activity activity) {
        if (!EglOverlay.isAttached()) MAIN.post(ActivityTrackerBridge::ensureOverlayAttached);
    }

    @Override
    public void onActivityStopped(Activity activity) {}

    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle outState) {}

    private static native void nativeOnResumed(Activity activity);

    private static native void nativeOnPaused(Activity activity);

    private static native void nativeOnDestroyed(Activity activity);
}
