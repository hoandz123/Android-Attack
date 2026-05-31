#include "Core/Bootstrap.h"
#include "UI/BridgeDex.h"
#include "UI/OverlaySurface.h"
#include "UI/TouchInput.h"
#include "UI/ImGuiRenderer.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

namespace bootstrap {

JavaVM *g_vm = nullptr;
jobject g_activity = nullptr;
std::atomic<long> g_nextId{1};
std::unordered_map<long, std::function<void()>> g_runCallbacks;
std::mutex g_bridgeClassMutex;
std::unordered_map<std::string, jclass> g_bridgeClassCache;

long nextId() { return g_nextId.fetch_add(1); }

jstring makeJString(JNIEnv *env, const char *text) { return env->NewStringUTF(text ? text : ""); }

std::string jstringToUtf8(JNIEnv *env, jstring js) {
    if (!js) return "";
    const char *utf = env->GetStringUTFChars(js, nullptr);
    std::string out(utf ? utf : "");
    if (utf) env->ReleaseStringUTFChars(js, utf);
    return out;
}

void initVm(JavaVM *vm) { g_vm = vm; }

JNIEnv *getEnv() {
    JNIEnv *env = nullptr;
    if (!g_vm) { LOGE(OBFUSCATE("getEnv: JavaVM not initialized")); return nullptr; }
    if (g_vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != 0) { LOGE(OBFUSCATE("getEnv: AttachCurrentThread failed")); return nullptr; }
    }
    return env;
}

bool checkException(JNIEnv *env, const char *where) {
    if (!env || !env->ExceptionCheck()) return false;
    jthrowable ex = env->ExceptionOccurred();
    env->ExceptionClear();
    if (!ex) { LOGE(OBFUSCATE("EXC @ %s (no throwable)"), where); return true; }
    std::string clsName, msg, trace;
    jobject exClsObj = env->CallObjectMethod(ex, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Object")), OBFUSCATE("getClass"), OBFUSCATE("()Ljava/lang/Class;")));
    if (exClsObj) {
        jstring nm = (jstring) env->CallObjectMethod(exClsObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getName"), OBFUSCATE("()Ljava/lang/String;")));
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (nm) { clsName = jstringToUtf8(env, nm); env->DeleteLocalRef(nm); }
    }
    jclass exClass = env->GetObjectClass(ex);
    if (exClass) {
        jmethodID getMsg = env->GetMethodID(exClass, OBFUSCATE("getMessage"), OBFUSCATE("()Ljava/lang/String;"));
        if (getMsg) {
            jstring jmsg = (jstring) env->CallObjectMethod(ex, getMsg);
            if (env->ExceptionCheck()) env->ExceptionClear();
            if (jmsg) { msg = jstringToUtf8(env, jmsg); env->DeleteLocalRef(jmsg); }
        }
    }
    jstring jtrace = (jstring) env->CallStaticObjectMethod(env->FindClass(OBFUSCATE("android/util/Log")), env->GetStaticMethodID(env->FindClass(OBFUSCATE("android/util/Log")), OBFUSCATE("getStackTraceString"), OBFUSCATE("(Ljava/lang/Throwable;)Ljava/lang/String;")), ex);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (jtrace) { trace = jstringToUtf8(env, jtrace); env->DeleteLocalRef(jtrace); }
    LOGE(OBFUSCATE("EXC @ %s | %s: %s"), where, clsName.c_str(), msg.c_str());
    if (!trace.empty()) LOGE(OBFUSCATE("%s"), trace.c_str());
    if (exClass) env->DeleteLocalRef(exClass);
    if (exClsObj) env->DeleteLocalRef(exClsObj);
    env->DeleteLocalRef(ex);
    return true;
}

jobject fetchCurrentActivityOnce(JNIEnv *env) {
    jclass atClass = env->FindClass(OBFUSCATE("android/app/ActivityThread"));
    if (!atClass || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: ActivityThread class")); return nullptr; }
    jmethodID currentAt = env->GetStaticMethodID(atClass, OBFUSCATE("currentActivityThread"), OBFUSCATE("()Landroid/app/ActivityThread;"));
    jobject at = env->CallStaticObjectMethod(atClass, currentAt);
    if (!at || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: currentActivityThread")); return nullptr; }
    jstring activitiesName = env->NewStringUTF(OBFUSCATE("mActivities"));
    jobject activitiesField = env->CallObjectMethod(atClass, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getDeclaredField"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/reflect/Field;")), activitiesName);
    env->DeleteLocalRef(activitiesName);
    if (!activitiesField || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: mActivities field")); return nullptr; }
    env->CallVoidMethod(activitiesField, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/reflect/Field")), OBFUSCATE("setAccessible"), OBFUSCATE("(Z)V")), JNI_TRUE);
    if (env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: setAccessible")); return nullptr; }
    jobject activitiesMap = env->CallObjectMethod(activitiesField, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/reflect/Field")), OBFUSCATE("get"), OBFUSCATE("(Ljava/lang/Object;)Ljava/lang/Object;")), at);
    if (!activitiesMap || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: mActivities get")); return nullptr; }
    jobject values = env->CallObjectMethod(activitiesMap, env->GetMethodID(env->FindClass(OBFUSCATE("java/util/Map")), OBFUSCATE("values"), OBFUSCATE("()Ljava/util/Collection;")));
    if (!values || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: Map.values")); return nullptr; }
    jobject iter = env->CallObjectMethod(values, env->GetMethodID(env->FindClass(OBFUSCATE("java/util/Collection")), OBFUSCATE("iterator"), OBFUSCATE("()Ljava/util/Iterator;")));
    if (!iter || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: iterator")); return nullptr; }
    jobject itObj = iter;
    jclass recordClass = env->FindClass(OBFUSCATE("android/app/ActivityThread$ActivityClientRecord"));
    jfieldID activityField = env->GetFieldID(recordClass, OBFUSCATE("activity"), OBFUSCATE("Landroid/app/Activity;"));
    jfieldID pausedField = env->GetFieldID(recordClass, OBFUSCATE("paused"), OBFUSCATE("Z"));
    jobject found = nullptr;
    while (env->CallBooleanMethod(itObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/util/Iterator")), OBFUSCATE("hasNext"), OBFUSCATE("()Z")))) {
        jobject record = env->CallObjectMethod(itObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/util/Iterator")), OBFUSCATE("next"), OBFUSCATE("()Ljava/lang/Object;")));
        if (!record || env->ExceptionCheck()) { checkException(env, OBFUSCATE("fetchCurrentActivityOnce: iterate")); break; }
        jboolean paused = env->GetBooleanField(record, pausedField);
        jobject activity = env->GetObjectField(record, activityField);
        if (!paused && activity != nullptr) { found = activity; break; }
    }
    if (!found) LOGE(OBFUSCATE("fetchCurrentActivityOnce: no foreground activity"));
    return found;
}

jobject getActivity(JNIEnv *env) {
    if (g_activity != nullptr) return g_activity;
    jobject local = fetchCurrentActivityOnce(env);
    if (local != nullptr) {
        g_activity = env->NewGlobalRef(local);
        env->DeleteLocalRef(local);
        LOGI(OBFUSCATE("getActivity: cached foreground activity"));
    }
    return g_activity;
}

jobject getContext() { return getActivity(getEnv()); }

jclass bridgeClass(JNIEnv *env, const char *dotName) {
    std::lock_guard<std::mutex> lock(g_bridgeClassMutex);
    auto it = g_bridgeClassCache.find(dotName);
    if (it != g_bridgeClassCache.end()) return it->second;
    jclass local = dexInject::loadBridgeClass(env, dotName);
    if (!local) { checkException(env, OBFUSCATE("bridgeClass: loadBridgeClass")); LOGE(OBFUSCATE("bridgeClass: %s not loaded"), dotName); return nullptr; }
    jclass global = (jclass) env->NewGlobalRef(local);
    g_bridgeClassCache[dotName] = global;
    env->DeleteLocalRef(local);
    return global;
}

namespace {
static void nativeRun(JNIEnv *env, jclass clazz, jlong id) {
    (void) env;
    (void) clazz;
    auto it = g_runCallbacks.find((long) id);
    if (it != g_runCallbacks.end() && it->second) it->second();
}

static void nativeOnError(JNIEnv *env, jclass clazz, jlong id, jstring trace) {
    (void) clazz;
    std::string t = jstringToUtf8(env, trace);
    LOGE(OBFUSCATE("UI thread exception in posted task (id=%ld):\n%s"), (long) id, t.c_str());
    g_runCallbacks.erase((long) id);
}

static jboolean nativeOnTouch(JNIEnv *env, jclass clazz, jlong id, jint action, jfloat rawX, jfloat rawY) {
    return touchInput::onNativeTouch(env, clazz, id, action, rawX, rawY);
}

static void nativeOnSurfaceReady(JNIEnv *env, jclass clazz, jobject surface, jint width, jint height) {
    overlaySurface::onSurfaceReady(env, clazz, surface, width, height);
}

static void nativeOnSurfaceChanged(JNIEnv *env, jclass clazz, jobject surface, jint width, jint height) {
    overlaySurface::onSurfaceChanged(env, clazz, surface, width, height);
}

static void nativeOnSurfaceDestroyed(JNIEnv *env, jclass clazz) {
    overlaySurface::onSurfaceDestroyed(env, clazz);
}

static void nativeSignalFrame(JNIEnv *env, jclass clazz) {
    (void) env;
    (void) clazz;
    imguiRenderer::signalFrame();
}

static std::atomic<bool> g_nativesRegistered{false};
}

int RegisterNativeBridge(JNIEnv *env) {
    if (g_nativesRegistered.load()) return JNI_OK;
    jobject activity = getActivity(env);
    if (!activity) { LOGE(OBFUSCATE("RegisterNativeBridge: no activity")); return JNI_ERR; }
    if (!dexInject::injectBridgeDex(env, activity)) { LOGE(OBFUSCATE("RegisterNativeBridge: inject failed")); return JNI_ERR; }
    JNINativeMethod methods[] = {
            {OBFUSCATE("nativeRun"), OBFUSCATE("(J)V"), (void *) nativeRun},
            {OBFUSCATE("nativeOnError"), OBFUSCATE("(JLjava/lang/String;)V"), (void *) nativeOnError},
            {OBFUSCATE("nativeOnTouch"), OBFUSCATE("(JIFF)Z"), (void *) nativeOnTouch},
            {OBFUSCATE("nativeOnSurfaceReady"), OBFUSCATE("(Landroid/view/Surface;II)V"), (void *) nativeOnSurfaceReady},
            {OBFUSCATE("nativeOnSurfaceChanged"), OBFUSCATE("(Landroid/view/Surface;II)V"), (void *) nativeOnSurfaceChanged},
            {OBFUSCATE("nativeOnSurfaceDestroyed"), OBFUSCATE("()V"), (void *) nativeOnSurfaceDestroyed},
            {OBFUSCATE("nativeSignalFrame"), OBFUSCATE("()V"), (void *) nativeSignalFrame},
    };
    jclass clazz = dexInject::loadBridgeClass(env, OBFUSCATE("com.android.attack.NativeBridge"));
    if (!clazz) { checkException(env, OBFUSCATE("RegisterNativeBridge: loadClass")); LOGE(OBFUSCATE("RegisterNativeBridge: loadClass failed")); return JNI_ERR; }
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0) { checkException(env, OBFUSCATE("RegisterNativeBridge: RegisterNatives")); LOGE(OBFUSCATE("RegisterNativeBridge: RegisterNatives failed")); return JNI_ERR; }
    g_nativesRegistered.store(true);
    LOGI(OBFUSCATE("RegisterNativeBridge: ok"));
    return JNI_OK;
}

void post(std::function<void()> fn) {
    long id = nextId();
    g_runCallbacks[id] = std::move(fn);
    JNIEnv *env = getEnv();
    if (!env) { LOGE(OBFUSCATE("bootstrap::post: no JNIEnv")); g_runCallbacks.erase(id); return; }
    jclass runClass = bridgeClass(env, OBFUSCATE("com.android.attack.NativeBridge$Run"));
    if (!runClass || env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::post: Run class")); g_runCallbacks.erase(id); return; }
    jmethodID runCtor = env->GetMethodID(runClass, OBFUSCATE("<init>"), OBFUSCATE("(J)V"));
    if (!runCtor || env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::post: Run ctor")); g_runCallbacks.erase(id); return; }
    jobject runnable = env->NewObject(runClass, runCtor, (jlong) id);
    if (!runnable || env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::post: Run instance")); g_runCallbacks.erase(id); return; }
    jobject activity = getActivity(env);
    if (!activity) { LOGE(OBFUSCATE("bootstrap::post: no activity")); g_runCallbacks.erase(id); return; }
    env->CallVoidMethod(activity, env->GetMethodID(env->FindClass(OBFUSCATE("android/app/Activity")), OBFUSCATE("runOnUiThread"), OBFUSCATE("(Ljava/lang/Runnable;)V")), runnable);
    if (env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::post: runOnUiThread failed")); g_runCallbacks.erase(id); }
}

bool run() {
    JNIEnv *env = getEnv();
    if (!env) { LOGE(OBFUSCATE("bootstrap::run: no JNIEnv")); return false; }
    for (int i = 0; i < 50; i++) {
        if (getActivity(env) != nullptr) break;
        usleep(100000);
    }
    if (g_activity == nullptr) {
        LOGE(OBFUSCATE("bootstrap::run: activity not available after poll"));
        return false;
    }
    if (RegisterNativeBridge(env) != JNI_OK) {
        LOGE(OBFUSCATE("bootstrap::run: RegisterNativeBridge failed"));
        return false;
    }
    return true;
}

int sdkInt(JNIEnv *env) {
    jclass build = env->FindClass(OBFUSCATE("android/os/Build$VERSION"));
    if (!build || env->ExceptionCheck()) { env->ExceptionClear(); return 19; }
    jfieldID f = env->GetStaticFieldID(build, OBFUSCATE("SDK_INT"), OBFUSCATE("I"));
    return env->GetStaticIntField(build, f);
}

jobject getField(JNIEnv *env, jobject target, const char *name) {
    jclass clazzObj = env->GetObjectClass(target);
    while (clazzObj != nullptr) {
        jstring jname = env->NewStringUTF(name);
        jobject fieldObj = env->CallObjectMethod(clazzObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getDeclaredField"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/reflect/Field;")), jname);
        env->DeleteLocalRef(jname);
        if (env->ExceptionCheck()) { env->ExceptionClear(); clazzObj = (jclass) env->CallObjectMethod(clazzObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getSuperclass"), OBFUSCATE("()Ljava/lang/Class;"))); continue; }
        if (fieldObj != nullptr) {
            env->CallVoidMethod(fieldObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/reflect/Field")), OBFUSCATE("setAccessible"), OBFUSCATE("(Z)V")), JNI_TRUE);
            if (env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::getField: setAccessible")); LOGE(OBFUSCATE("bootstrap::getField: setAccessible %s"), name); return nullptr; }
            jobject val = env->CallObjectMethod(fieldObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/reflect/Field")), OBFUSCATE("get"), OBFUSCATE("(Ljava/lang/Object;)Ljava/lang/Object;")), target);
            if (env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::getField: get")); LOGE(OBFUSCATE("bootstrap::getField: get %s"), name); return nullptr; }
            return val;
        }
        clazzObj = (jclass) env->CallObjectMethod(clazzObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getSuperclass"), OBFUSCATE("()Ljava/lang/Class;")));
    }
    LOGE(OBFUSCATE("bootstrap::getField: %s not found"), name);
    return nullptr;
}

jobject getMethod(JNIEnv *env, jobject target, const char *name, jobjectArray paramTypes) {
    jclass clazzObj = env->GetObjectClass(target);
    while (clazzObj != nullptr) {
        jstring jname = env->NewStringUTF(name);
        jobject methodObj = env->CallObjectMethod(clazzObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getDeclaredMethod"), OBFUSCATE("(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;")), jname, paramTypes);
        env->DeleteLocalRef(jname);
        if (env->ExceptionCheck()) { env->ExceptionClear(); clazzObj = (jclass) env->CallObjectMethod(clazzObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getSuperclass"), OBFUSCATE("()Ljava/lang/Class;"))); continue; }
        if (methodObj != nullptr) {
            env->CallVoidMethod(methodObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/reflect/Method")), OBFUSCATE("setAccessible"), OBFUSCATE("(Z)V")), JNI_TRUE);
            if (env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::getMethod: setAccessible")); LOGE(OBFUSCATE("bootstrap::getMethod: setAccessible %s"), name); return nullptr; }
            return methodObj;
        }
        clazzObj = (jclass) env->CallObjectMethod(clazzObj, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/Class")), OBFUSCATE("getSuperclass"), OBFUSCATE("()Ljava/lang/Class;")));
    }
    LOGE(OBFUSCATE("bootstrap::getMethod: %s not found"), name);
    return nullptr;
}

jobject classLoaderOf(JNIEnv *env, jobject activity) {
    jobject loader = env->CallObjectMethod(activity, env->GetMethodID(env->FindClass(OBFUSCATE("android/app/Activity")), OBFUSCATE("getClassLoader"), OBFUSCATE("()Ljava/lang/ClassLoader;")));
    if (!loader || env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::classLoaderOf: getClassLoader")); return nullptr; }
    return loader;
}

jclass loadClass(JNIEnv *env, jobject loader, const char *dotName) {
    jstring jname = env->NewStringUTF(dotName);
    jobject clsObj = env->CallObjectMethod(loader, env->GetMethodID(env->FindClass(OBFUSCATE("java/lang/ClassLoader")), OBFUSCATE("loadClass"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/Class;")), jname);
    env->DeleteLocalRef(jname);
    if (!clsObj || env->ExceptionCheck()) { checkException(env, OBFUSCATE("bootstrap::loadClass")); LOGE(OBFUSCATE("bootstrap::loadClass %s"), dotName); return nullptr; }
    return (jclass) clsObj;
}

}

namespace dexInject {
static jobject g_hostLoader = nullptr;

bool injectBridgeDex(JNIEnv *env, jobject activity) {
    if (g_hostLoader != nullptr) return true;
    if (kBridgeDexLen == 0) { LOGE(OBFUSCATE("dexInject: empty bridge dex (run genBridgeHeader)")); return false; }
    if (bootstrap::sdkInt(env) < 26) { LOGE(OBFUSCATE("dexInject: makeInMemoryDexElements requires API 26+")); return false; }
    jobject loader = bootstrap::classLoaderOf(env, activity);
    if (!loader) return false;
    jobject pathList = bootstrap::getField(env, loader, OBFUSCATE("pathList"));
    if (!pathList) return false;
    jobject dexElementsField = nullptr;
    {
        jclass classClass = env->FindClass(OBFUSCATE("java/lang/Class"));
        jmethodID getDeclaredField = env->GetMethodID(classClass, OBFUSCATE("getDeclaredField"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/reflect/Field;"));
        jclass fieldClass = env->FindClass(OBFUSCATE("java/lang/reflect/Field"));
        jmethodID setAccessible = env->GetMethodID(fieldClass, OBFUSCATE("setAccessible"), OBFUSCATE("(Z)V"));
        jclass clazz = env->GetObjectClass(pathList);
        while (clazz != nullptr) {
            jstring jname = env->NewStringUTF(OBFUSCATE("dexElements"));
            dexElementsField = env->CallObjectMethod(clazz, getDeclaredField, jname);
            env->DeleteLocalRef(jname);
            if (env->ExceptionCheck()) { env->ExceptionClear(); clazz = env->GetSuperclass(clazz); continue; }
            if (dexElementsField != nullptr) {
                env->CallVoidMethod(dexElementsField, setAccessible, JNI_TRUE);
                break;
            }
            clazz = env->GetSuperclass(clazz);
        }
    }
    if (!dexElementsField) { LOGE(OBFUSCATE("dexInject: dexElements field")); return false; }
    jclass fieldClass = env->FindClass(OBFUSCATE("java/lang/reflect/Field"));
    jmethodID fieldGet = env->GetMethodID(fieldClass, OBFUSCATE("get"), OBFUSCATE("(Ljava/lang/Object;)Ljava/lang/Object;"));
    jmethodID fieldSet = env->GetMethodID(fieldClass, OBFUSCATE("set"), OBFUSCATE("(Ljava/lang/Object;Ljava/lang/Object;)V"));
    jobject dexElements = env->CallObjectMethod(dexElementsField, fieldGet, pathList);
    if (!dexElements || env->ExceptionCheck()) { bootstrap::checkException(env, OBFUSCATE("dexInject: get dexElements")); return false; }
    jclass byteBufferClass = env->FindClass(OBFUSCATE("java/nio/ByteBuffer"));
    jobject buffer = env->NewDirectByteBuffer((void *) kBridgeDex, (jlong) kBridgeDexLen);
    if (!buffer || env->ExceptionCheck()) { bootstrap::checkException(env, OBFUSCATE("dexInject: NewDirectByteBuffer")); return false; }
    jobjectArray buffers = env->NewObjectArray(1, byteBufferClass, buffer);
    jclass arrayListClass = env->FindClass(OBFUSCATE("java/util/ArrayList"));
    jmethodID arrayListCtor = env->GetMethodID(arrayListClass, OBFUSCATE("<init>"), OBFUSCATE("()V"));
    jobject suppressed = env->NewObject(arrayListClass, arrayListCtor);
    jclass classClass = env->FindClass(OBFUSCATE("java/lang/Class"));
    jclass byteBufferArrayClass = env->FindClass(OBFUSCATE("[Ljava/nio/ByteBuffer;"));
    jclass listClass = env->FindClass(OBFUSCATE("java/util/List"));
    jobjectArray paramTypes = env->NewObjectArray(2, classClass, nullptr);
    env->SetObjectArrayElement(paramTypes, 0, byteBufferArrayClass);
    env->SetObjectArrayElement(paramTypes, 1, listClass);
    jobject makeDexMethod = bootstrap::getMethod(env, pathList, OBFUSCATE("makeInMemoryDexElements"), paramTypes);
    if (!makeDexMethod) return false;
    jclass methodClass = env->FindClass(OBFUSCATE("java/lang/reflect/Method"));
    jmethodID methodInvoke = env->GetMethodID(methodClass, OBFUSCATE("invoke"), OBFUSCATE("(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;"));
    jobjectArray invokeArgs = env->NewObjectArray(2, env->FindClass(OBFUSCATE("java/lang/Object")), nullptr);
    env->SetObjectArrayElement(invokeArgs, 0, buffers);
    env->SetObjectArrayElement(invokeArgs, 1, suppressed);
    jobject addElements = env->CallObjectMethod(makeDexMethod, methodInvoke, pathList, invokeArgs);
    if (!addElements || env->ExceptionCheck()) { bootstrap::checkException(env, OBFUSCATE("dexInject: makeInMemoryDexElements invoke")); return false; }
    jclass arrayClass = env->FindClass(OBFUSCATE("java/lang/reflect/Array"));
    jmethodID getLength = env->GetStaticMethodID(arrayClass, OBFUSCATE("getLength"), OBFUSCATE("(Ljava/lang/Object;)I"));
    jint oldLen = env->CallStaticIntMethod(arrayClass, getLength, dexElements);
    jint addLen = env->CallStaticIntMethod(arrayClass, getLength, addElements);
    jclass componentClass = oldLen > 0 ? env->GetObjectClass(env->GetObjectArrayElement((jobjectArray) dexElements, 0)) : env->GetObjectClass(env->GetObjectArrayElement((jobjectArray) addElements, 0));
    jmethodID newInstance = env->GetStaticMethodID(arrayClass, OBFUSCATE("newInstance"), OBFUSCATE("(Ljava/lang/Class;I)Ljava/lang/Object;"));
    jobject newElements = env->CallStaticObjectMethod(arrayClass, newInstance, componentClass, oldLen + addLen);
    jclass systemClass = env->FindClass(OBFUSCATE("java/lang/System"));
    jmethodID arraycopy = env->GetStaticMethodID(systemClass, OBFUSCATE("arraycopy"), OBFUSCATE("(Ljava/lang/Object;ILjava/lang/Object;II)V"));
    env->CallStaticVoidMethod(systemClass, arraycopy, dexElements, 0, newElements, 0, oldLen);
    env->CallStaticVoidMethod(systemClass, arraycopy, addElements, 0, newElements, oldLen, addLen);
    env->CallVoidMethod(dexElementsField, fieldSet, pathList, newElements);
    if (env->ExceptionCheck()) { bootstrap::checkException(env, OBFUSCATE("dexInject: set dexElements")); return false; }
    g_hostLoader = env->NewGlobalRef(loader);
    LOGI(OBFUSCATE("dexInject: bridge dex injected (%u bytes)"), (unsigned) kBridgeDexLen);
    return true;
}

jclass loadBridgeClass(JNIEnv *env, const char *dotName) {
    if (!g_hostLoader) { LOGE(OBFUSCATE("dexInject: loadBridgeClass before inject")); return nullptr; }
    return bootstrap::loadClass(env, g_hostLoader, dotName);
}
}
