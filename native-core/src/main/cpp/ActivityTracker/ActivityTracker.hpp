#pragma once

#include <jni.h>
#include <vector>

namespace activity_tracker {

// JNI_OnLoad: cần jni::Init(vm) trước; đăng ký bridge + Java install().
bool Init(JavaVM *vm);

std::vector<jobject> GetActivities(JNIEnv *env);
jobject CurrentActivity(JNIEnv *env);

void OnActivityResumed(JNIEnv *env, jobject activity);
void OnActivityPaused(JNIEnv *env, jobject activity);
void OnActivityDestroyed(JNIEnv *env, jobject activity);

} // namespace activity_tracker
