package com.android.attack.nativedex;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;
import android.util.ArrayMap;
import android.util.Log;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Collection;

public final class ActivityTrackerBridge implements Application.ActivityLifecycleCallbacks {

    private static final String TAG = "ActivityTracker";
    private static volatile boolean installed;
    private static ActivityTrackerBridge instance;

    public static boolean install() {
        if (installed) {
            syncExistingActivities();
            return true;
        }
        synchronized (ActivityTrackerBridge.class) {
            if (installed) {
                syncExistingActivities();
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

    private static void syncExistingActivities() {
        try {
            Class<?> at = Class.forName("android.app.ActivityThread");
            Method m = at.getDeclaredMethod("currentActivityThread");
            m.setAccessible(true);
            Object thread = m.invoke(null);
            if (thread == null) return;
            Field f = at.getDeclaredField("mActivities");
            f.setAccessible(true);
            if (!(f.get(thread) instanceof ArrayMap)) return;
            ArrayMap<?, ?> map = (ArrayMap<?, ?>) f.get(thread);
            int n = 0;
            for (Object rec : map.values()) {
                if (rec == null) continue;
                Field paused = rec.getClass().getDeclaredField("paused");
                paused.setAccessible(true);
                if (paused.getBoolean(rec)) continue;
                Field actF = rec.getClass().getDeclaredField("activity");
                actF.setAccessible(true);
                Object a = actF.get(rec);
                if (a instanceof Activity) {
                    dispatchResumed((Activity) a);
                    n++;
                }
            }
            if (n > 0) Log.i(TAG, "synced " + n + " activities");
        } catch (Throwable t) {
            Log.w(TAG, "sync", t);
        }
    }

    private static void dispatchResumed(Activity a) {
        if (a == null) return;
        try {
            Log.i(TAG, "resumed " + a.getClass().getName());
            nativeOnResumed(a);
            TouchInputBridge.install(a);
            KeyboardInputBridge.install(a);
            EglOverlay.onActivityResumed(a);
            TouchInputBridge.refreshInsets(a);
        } catch (Throwable t) {
            Log.e(TAG, "resume", t);
        }
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
    public void onActivityStarted(Activity activity) {}

    @Override
    public void onActivityStopped(Activity activity) {}

    @Override
    public void onActivitySaveInstanceState(Activity activity, Bundle outState) {}

    private static native void nativeOnResumed(Activity activity);

    private static native void nativeOnPaused(Activity activity);

    private static native void nativeOnDestroyed(Activity activity);
}
