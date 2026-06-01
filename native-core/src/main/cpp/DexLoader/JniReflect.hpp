#pragma once

#include <jni.h>

namespace jni_reflect {

jobject FindFieldObject(JNIEnv *env, jobject instance, const char *name);
bool HasFieldObject(JNIEnv *env, jobject instance, const char *name);
jobject FindMethod(JNIEnv *env, jobject instance, const char *name, const char *signature);

} // namespace jni_reflect
