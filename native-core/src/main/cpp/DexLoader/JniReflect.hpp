#pragma once

#include <jni.h>

namespace jni_reflect {

jobject find_field_object(JNIEnv *env, jobject instance, const char *name);
bool has_field_object(JNIEnv *env, jobject instance, const char *name);
jobject find_method(JNIEnv *env, jobject instance, const char *name, const char *signature);

} // namespace jni_reflect
