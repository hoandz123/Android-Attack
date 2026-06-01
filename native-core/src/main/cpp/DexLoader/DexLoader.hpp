#pragma once

#include <jni.h>
#include <cstddef>
#include <vector>

namespace dex_loader {

// JNI_OnLoad: inject embedded DEX into Application ClassLoader.
bool init(JavaVM *vm, const uint8_t *dex, size_t size);

// Inject in-memory DEX into the ClassLoader of |context| (makeInMemoryDexElements).
bool load_into_context(JNIEnv *env, jobject context, const uint8_t *dex, size_t size);
bool load_into_context(JNIEnv *env, jobject context, const std::vector<std::vector<uint8_t>> &dex_files);

} // namespace dex_loader
