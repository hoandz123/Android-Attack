#pragma once

#include <jni.h>
#include <vector>

namespace activity_tracker {

// JNI_OnLoad: cần jni::init(vm) trước; đăng ký bridge + Java install().
bool init(JavaVM *vm);

std::vector<jobject> activities(JNIEnv *env);
jobject current_activity(JNIEnv *env);

void on_activity_resumed(JNIEnv *env, jobject activity);
void on_activity_paused(JNIEnv *env, jobject activity);
void on_activity_destroyed(JNIEnv *env, jobject activity);

} // namespace activity_tracker
