#pragma once

#include <jni.h>

namespace attack::loader {

bool bootstrap(JavaVM *vm, void *reserved);
bool loadBuiltinPlugins(JNIEnv *env);
bool loadFromDir(JNIEnv *env, const char *nativeLibraryDir, const char *libName);
bool loadDownloaded(JNIEnv *env, const char *absolutePath);

} // namespace attack::loader
