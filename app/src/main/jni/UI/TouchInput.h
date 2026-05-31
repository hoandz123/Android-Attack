#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <jni.h>

namespace touchInput {

void applyPendingTouch();
void syncInputWindowAfterFrame();
void setCaptureRect(float x, float y, float w, float h);
void clearCaptureRect();
jboolean onNativeTouch(JNIEnv *env, jclass clazz, jlong id, jint action, jfloat rawX, jfloat rawY);
bool wantCaptureMouse();

}

#endif
