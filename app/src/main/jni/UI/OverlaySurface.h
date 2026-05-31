#ifndef OVERLAY_SURFACE_H
#define OVERLAY_SURFACE_H

#include <jni.h>
#include <android/native_window.h>

namespace overlaySurface {

bool create();
bool waitReady(int timeoutMs);
ANativeWindow *getWindow();
int getWidth();
int getHeight();
void onSurfaceReady(JNIEnv *env, jclass clazz, jobject surface, jint width, jint height);
void onSurfaceChanged(JNIEnv *env, jclass clazz, jobject surface, jint width, jint height);
void onSurfaceDestroyed(JNIEnv *env, jclass clazz);

}

#endif
