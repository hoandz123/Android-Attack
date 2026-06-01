#pragma once

#include <jni.h>
#include <cstddef>
#include <vector>

namespace dex_loader {

// JNI_OnLoad: inject embedded DEX into Application ClassLoader.
bool Init(JavaVM *vm, const uint8_t *dex, size_t size);

// Inject in-memory DEX into the ClassLoader of |context| (makeInMemoryDexElements).
bool LoadIntoContext(JNIEnv *env, jobject context, const uint8_t *dex, size_t size);
bool LoadIntoContext(JNIEnv *env, jobject context, const std::vector<std::vector<uint8_t>> &dex_files);

} // namespace dex_loader
