#pragma once

#include <jni.h>

namespace attack::loader {

bool bootstrap(JavaVM *vm, void *reserved);
bool loadFromDir(const char *nativeLibraryDir, const char *libName);
bool loadDownloaded(const char *absolutePath);

} // namespace attack::loader
