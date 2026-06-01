#pragma once

#include <jni.h>

namespace jni {

constexpr jint kVersion = JNI_VERSION_1_6;

/** Gọi một lần từ JNI_OnLoad (trước dex/tracker/mod-ui). */
bool init(JavaVM *vm);

bool inited();
JavaVM *vm();

/** JNIEnv của thread hiện tại nếu đã attach (JNI_OK). Không Attach/Detach. */
JNIEnv *env();

/**
 * RAII: dùng env sẵn có; chỉ Attach khi thread native chưa gắn JVM.
 * Destructor Detach chỉ khi lần này đã Attach — an toàn với JNI callback / render thread.
 */
class ScopedEnv {
public:
    ScopedEnv();
    ~ScopedEnv();

    ScopedEnv(const ScopedEnv &) = delete;
    ScopedEnv &operator=(const ScopedEnv &) = delete;

    JNIEnv *get() const { return env_; }
    bool ok() const { return env_ != nullptr; }
    JNIEnv *operator->() const { return env_; }

private:
    JNIEnv *env_ = nullptr;
    bool attached_ = false;
};

void clear_exception(JNIEnv *env);

/** FindClass hoặc load qua Application ClassLoader (embedded dex). */
jclass find_class(JNIEnv *env, const char *slash_name);

bool register_natives(JNIEnv *env, const char *slash_class, const JNINativeMethod *methods,
                      jint count);

} // namespace jni
