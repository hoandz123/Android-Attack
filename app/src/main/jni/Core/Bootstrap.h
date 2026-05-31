#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include <jni.h>
#include <functional>

namespace bootstrap {

void initVm(JavaVM *vm);
JNIEnv *getEnv();
bool checkException(JNIEnv *env, const char *where);
jobject getActivity(JNIEnv *env);
jobject getContext();
bool run();
void post(std::function<void()> fn);
int RegisterNativeBridge(JNIEnv *env);
int sdkInt(JNIEnv *env);
jobject getField(JNIEnv *env, jobject target, const char *name);
jobject getMethod(JNIEnv *env, jobject target, const char *name, jobjectArray paramTypes);
jobject classLoaderOf(JNIEnv *env, jobject activity);
jclass loadClass(JNIEnv *env, jobject loader, const char *dotName);

}

namespace dexInject {
bool injectBridgeDex(JNIEnv *env, jobject activity);
jclass loadBridgeClass(JNIEnv *env, const char *dotName);
}

#endif
