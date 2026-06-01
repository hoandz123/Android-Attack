#include "JniReflect.hpp"

#include <android/log.h>
#include <cstring>

#define TAG "JniReflect"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace jni_reflect {

static jclass class_class(JNIEnv *env) {
    static jclass cached = nullptr;
    if (!cached) cached = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("java/lang/Class")));
    return cached;
}

static void set_accessible(JNIEnv *env, jobject member) {
    jclass cls = env->GetObjectClass(member);
    jmethodID set = env->GetMethodID(cls, "setAccessible", "(Z)V");
    env->CallVoidMethod(member, set, JNI_TRUE);
}

bool has_field_object(JNIEnv *env, jobject instance, const char *name) {
    if (!instance || !name) return false;
    jclass cursor = reinterpret_cast<jclass>(env->NewLocalRef(env->GetObjectClass(instance)));
    jmethodID get_declared = env->GetMethodID(class_class(env), "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jstring jname = env->NewStringUTF(name);

    while (cursor) {
        jobject field = env->CallObjectMethod(cursor, get_declared, jname);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else if (field) {
            env->DeleteLocalRef(field);
            env->DeleteLocalRef(cursor);
            env->DeleteLocalRef(jname);
            return true;
        }
        jclass parent = env->GetSuperclass(cursor);
        env->DeleteLocalRef(cursor);
        cursor = parent;
    }
    env->DeleteLocalRef(jname);
    return false;
}

jobject find_field_object(JNIEnv *env, jobject instance, const char *name) {
    if (!instance || !name) return nullptr;
    jclass cursor = reinterpret_cast<jclass>(env->NewLocalRef(env->GetObjectClass(instance)));
    jmethodID get_declared = env->GetMethodID(class_class(env), "getDeclaredField", "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
    jstring jname = env->NewStringUTF(name);

    while (cursor) {
        jobject field = env->CallObjectMethod(cursor, get_declared, jname);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else if (field) {
            set_accessible(env, field);
            env->DeleteLocalRef(cursor);
            env->DeleteLocalRef(jname);
            return field;
        }
        jclass parent = env->GetSuperclass(cursor);
        env->DeleteLocalRef(cursor);
        cursor = parent;
    }
    env->DeleteLocalRef(jname);
    LOGE("field not found: %s", name);
    return nullptr;
}

jobject find_method(JNIEnv *env, jobject instance, const char *name, const char *signature) {
    if (!instance || !name) return nullptr;
    jclass cursor = reinterpret_cast<jclass>(env->NewLocalRef(env->GetObjectClass(instance)));
    jmethodID get_declared = env->GetMethodID(class_class(env), "getDeclaredMethod",
                                                "(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;");
    jmethodID get_methods = env->GetMethodID(class_class(env), "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jmethodID get_name = env->GetMethodID(env->FindClass("java/lang/reflect/Method"), "getName", "()Ljava/lang/String;");
    jstring jname = env->NewStringUTF(name);

    while (cursor) {
        jobjectArray methods = reinterpret_cast<jobjectArray>(env->CallObjectMethod(cursor, get_methods));
        if (!env->ExceptionCheck() && methods) {
            const jsize n = env->GetArrayLength(methods);
            for (jsize i = 0; i < n; ++i) {
                jobject method = env->GetObjectArrayElement(methods, i);
                jstring mname = reinterpret_cast<jstring>(env->CallObjectMethod(method, get_name));
                const char *utf = env->GetStringUTFChars(mname, nullptr);
                const bool match = utf && strcmp(utf, name) == 0;
                env->ReleaseStringUTFChars(mname, utf);
                if (match) {
                    set_accessible(env, method);
                    env->DeleteLocalRef(methods);
                    env->DeleteLocalRef(cursor);
                    env->DeleteLocalRef(jname);
                    return method;
                }
            }
        } else if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }

        jobject method = env->CallObjectMethod(cursor, get_declared, jname, nullptr);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else if (method) {
            set_accessible(env, method);
            (void)signature;
            env->DeleteLocalRef(cursor);
            env->DeleteLocalRef(jname);
            return method;
        }

        jclass parent = env->GetSuperclass(cursor);
        env->DeleteLocalRef(cursor);
        cursor = parent;
    }
    env->DeleteLocalRef(jname);
    LOGE("method not found: %s", name);
    return nullptr;
}

} // namespace jni_reflect
