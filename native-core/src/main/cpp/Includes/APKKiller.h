/*
 * APKKiller — chữ ký (kill sign)
 * Nếu không xử lý / patch chữ ký được: bắt buộc crash process (abort), không cho flow vào game
 * tiếp tục với trạng thái nửa vời (dễ bị phát hiện). Log LOGE trước abort chỉ để debug.
 */
#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <jni.h>
#include <thread>
#include <future>
#include <unordered_map>
#include <map>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include "Logger.h"
#include "base64/base64.h"
#include "obfuscate.h"

#define ApkKillSignFail(Reason) \
    do { \
        LOGE("APKKiller kill sign fail: %s", Reason); \
        std::abort(); \
    } while (0)

#define apk_asset_path "base.apk"
#define apk_fake_name "base.apk"

std::map<std::string, std::string> apk_signatures =
        {{"com.vng.playtogether",
                 "MIIDfTCCAmWgAwIBAgIEb5rakTANBgkqhkiG9w0BAQsFADBvMQswCQYDVQQGEwJWTjEUMBIGA1UE\nCBMLSG8gQ2hpIE1pbmgxFDASBgNVBAcTC0hvIENoaSBNaW5oMRIwEAYDVQQKEwlWTkcgR2FtZXMx\nEjAQBgNVBAsTCVZORyBHYW1lczEMMAoGA1UEAxMDVk5HMB4XDTE0MTIyNDEzNDgxNFoXDTQyMDUx\nMTEzNDgxNFowbzELMAkGA1UEBhMCVk4xFDASBgNVBAgTC0hvIENoaSBNaW5oMRQwEgYDVQQHEwtI\nbyBDaGkgTWluaDESMBAGA1UEChMJVk5HIEdhbWVzMRIwEAYDVQQLEwlWTkcgR2FtZXMxDDAKBgNV\nBAMTA1ZORzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAIpndgqwWdg5dukiA7FX6evL\nEn2TJsLG7g9VACD1YNVxoMUwwrh82XKfE+r3Q52lqU4895eb7OBzJOWVLdHo1TcvPFi1fM4huUbJ\n1FEti8NT0d3CzyO6ryvsfKyDaKkaYWENQFwCflMEaEfVaxJA/EsmY7LemoTxrdWS+Rq38xYipnBx\noV0xy0mnNZxcMtSTBa+k308kNULsO3x9qAy/Nrd3ldTMkZnMYNa3hoYzz2VbuMbgiXUNXSh61j5I\nXOhpIqWlXNdp8D0V/O3zdi39Dsv+nOPIOEv4z99a7eWn5kH8zXAqFjP1872dAzcq92p/hFam6vZa\nhNpI3nn1C1H2lUcCAwEAAaMhMB8wHQYDVR0OBBYEFFf9M3bvO9F23lDuKfKD9X0JM6hEMA0GCSqG\nSIb3DQEBCwUAA4IBAQB8hT7zuSixDMnfID3ilUapd+pHTHEcFgbeYprpVC0J818jFasgCunqGUsZ\nObM4re8wZ034slbfNKDgbNjx8ekChxnOsEiJxhYPpZW6zF64Ce6OI/FyYdgechbGcz/LnuZ0D7Ej\nkL5mrQ+svPypwZQJVe1RzFRQH7q8VyvDrA9sqq9FNsLvJNmsbvqqU1WoqOfrVw+MtMiDH1QYPuie\nSeP/29++NrRq66sLfYy7bSgZjuoIdUklJuaTV0Eg4ePYQzeeeTOCzFJWZBDfYfkosoOd0xQUIW0u\ntaPq3mVckLfOCwEvM4SulL7Xjg1VsIh8/mnOIU2twPL/JVLvOK20VkLV\n"},
         {"com.haegin.playtogether",
                 "MIICsDCCAZigAwIBAgIEfQMQsjANBgkqhkiG9w0BAQUFADAZMRcwFQYDVQQKDA5IYWVnaW4gQ28g\nLEx0ZDAgFw0xODA5MjcxMDMxMTlaGA8yMDY4MDkxNDEwMzExOVowGTEXMBUGA1UECgwOSGFlZ2lu\nIENvICxMdGQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQChq6HPo2A2Uwp8m02q4ims\nV/o5XGGH6PU+Bp3XkgCwF3ZvqQSzreTNsYIZKYMiMl4dssJPbRahUal5OhWu/QuvNCEvtu3+3f+E\nHD4poRSq0ZrR0PiFuOIBDEOdSde8ZSsoFZgjQW3h1coF3XKV79hr09ScwbEKBY/nDtA4IxAVkGyi\n/5dEQp29l/+cLLFrB/vCwC252CYb9JjA+owIxzUOWGY+wySq5tgPSDZZw7NnPfxRvKooqdjQyc/Q\nY1KiiO9x2tHo09FhqbImkLButIesrNgDhQeQloeE79QwGUaVcDNnYNeT2xDSdlUoqLY0fjoufpr4\nmwJI+nng8hByP4BBAgMBAAEwDQYJKoZIhvcNAQEFBQADggEBAJQzHg4lMeS1jaEgaQte3Zzzu5Js\nvP2COfqPl2u+hzsDnXGNIqPVsOJlLIT0IFXKsqQegeqQsGc4NO/lLJQr/LHwRK6Myz3exRM3+DvH\nEDQ91SX9t6hvSDzghcBK1Wlyf2HebYwgaW+DHzt7Gequ0ep+AowO59tGTlCGO/7dueI6Qpjsslka\n9FgQIffwkk7IlyI1bdagmUBOvdE4CfE9SZo+Zg6n4VgTdQCMp/fjmg7ZBebSUhBQET3tU18wKMDs\nm8TRD8kzxYip3e/dVOuLrdCbWsRTFU1rJCloLzzgrPIKnu+cECe10OY0bODXQnEKPqWNMKUWv6VM\ntF4XLySs/Tc=\n"}};


JavaVM *g_vm;
std::string g_pkgName;
jstring g_apkPath;
jobject g_proxy, g_pkgMgr;
int g_sdkVersion = 0;

void initSdkVersion(JNIEnv *env) {
    auto versionClass = env->FindClass("android/os/Build$VERSION");
    auto sdkIntField = env->GetStaticFieldID(versionClass, "SDK_INT", "I");
    g_sdkVersion = env->GetStaticIntField(versionClass, sdkIntField);
    LOGI("-------- Android SDK Version: %d", g_sdkVersion);
}

const char *getProcessName() {
    FILE *f = fopen("/proc/self/cmdline", "rb");
    if (f) {
        char *buf = new char[64];
        fread(buf, sizeof(char), 64, f);
        fclose(f);
        return buf;
    }
    return 0;
}

std::string getPackageName(jobject obj) {
    
    auto contextClass = JNIHelper::GetEnv()->FindClass("android/content/Context");
    auto getPackageNameMethod = JNIHelper::GetEnv()->GetMethodID(contextClass, "getPackageName",
                                                   "()Ljava/lang/String;");
    return JNIHelper::GetEnv()->GetStringUTFChars((jstring) JNIHelper::GetEnv()->CallObjectMethod(obj, getPackageNameMethod),
                                    0);
}

jobject getApplicationContext(jobject obj) {
    auto contextClass = JNIHelper::GetEnv()->FindClass("android/content/Context");
    auto getApplicationContextMethod = JNIHelper::GetEnv()->GetMethodID(contextClass, "getApplicationContext",
                                                          "()Landroid/content/Context;");
    return JNIHelper::GetEnv()->CallObjectMethod(obj, getApplicationContextMethod);
}

jobject getPackageManager(jobject obj) {
    auto contextClass = JNIHelper::GetEnv()->FindClass("android/content/Context");
    auto getPackageManagerMethod = JNIHelper::GetEnv()->GetMethodID(contextClass, "getPackageManager",
                                                      "()Landroid/content/pm/PackageManager;");
    return JNIHelper::GetEnv()->CallObjectMethod(obj, getPackageManagerMethod);
}

class Reference {
public:
    JNIEnv *env;
    jobject reference;
public:
    Reference(JNIEnv *env, jobject obj) {
        this->env = env;
        this->reference = env->NewGlobalRef(obj);
    }

    Reference(jobject reference) {
        this->env = JNIHelper::GetEnv();
        this->reference = reference;
    }

    jobject get() {
        auto referenceClass = env->FindClass("java/lang/ref/Reference");
        auto get = env->GetMethodID(referenceClass, "get", "()Ljava/lang/Object;");
        return env->CallObjectMethod(reference, get);
    }
};

class WeakReference : public Reference {
public:
    WeakReference(JNIEnv *env, jobject weakReference) : Reference(env, weakReference) {
    }

    WeakReference(jobject weakReference) : Reference(weakReference) {
    }

    static jobject Create(jobject obj) {
        auto weakReferenceClass = JNIHelper::GetEnv()->FindClass("java/lang/ref/WeakReference");
        auto weakReferenceClassConstructor = JNIHelper::GetEnv()->GetMethodID(weakReferenceClass, "<init>",
                                                                "(Ljava/lang/Object;)V");
        return JNIHelper::GetEnv()->NewObject(weakReferenceClass, weakReferenceClassConstructor, obj);
    }
};

class ArrayList {
private:
    JNIEnv *env;
    jobject arrayList;
public:
    ArrayList(JNIEnv *env, jobject arrayList) {
        this->env = env;
        this->arrayList = arrayList;
    }

    ArrayList(jobject arrayList) {
        this->env = JNIHelper::GetEnv();
        this->arrayList = arrayList;
    }

    jobject getObj() {
        return arrayList;
    }

    jobject get(int index) {
        auto arrayListClass = env->FindClass("java/util/ArrayList");
        auto getMethod = env->GetMethodID(arrayListClass, "get", "(I)Ljava/lang/Object;");
        return env->CallObjectMethod(arrayList, getMethod, index);
    }

    void set(int index, jobject value) {
        auto arrayListClass = env->FindClass("java/util/ArrayList");
        auto setMethod = env->GetMethodID(arrayListClass, "set",
                                          "(ILjava/lang/Object;)Ljava/lang/Object;");
        env->CallObjectMethod(arrayList, setMethod, index, value);
    }

    int size() {
        auto arrayListClass = env->FindClass("java/util/ArrayList");
        auto sizeMethod = env->GetMethodID(arrayListClass, "size", "()I");
        return env->CallIntMethod(arrayList, sizeMethod);
    }
};

class ArrayMap {
private:
    JNIEnv *env;
    jobject arrayMap;
public:
    ArrayMap(JNIEnv *env, jobject arrayMap) {
        this->env = env;
        this->arrayMap = arrayMap;
    }

    ArrayMap(jobject arrayMap) {
        this->env = JNIHelper::GetEnv();
        this->arrayMap = arrayMap;
    }

    jobject getObj() {
        return arrayMap;
    }

    jobject valueAt(int index) {
        auto arrayMapClass = env->FindClass("android/util/ArrayMap");
        auto valueAtMethod = env->GetMethodID(arrayMapClass, "valueAt", "(I)Ljava/lang/Object;");
        return env->CallObjectMethod(arrayMap, valueAtMethod, index);
    }

    jobject setValueAt(int index, jobject value) {
        auto arrayMapClass = env->FindClass("android/util/ArrayMap");
        auto setValueAtMethod = env->GetMethodID(arrayMapClass, "setValueAt",
                                                 "(ILjava/lang/Object;)Ljava/lang/Object;");
        return env->CallObjectMethod(arrayMap, setValueAtMethod, index, value);
    }

    int size() {
        auto arrayMapClass = env->FindClass("android/util/ArrayMap");
        auto sizeMethod = env->GetMethodID(arrayMapClass, "size", "()I");
        return env->CallIntMethod(arrayMap, sizeMethod);
    }
};

class Method {
private:
    JNIEnv *env;
    jobject method;
    jmethodID getNameMethod;
    jmethodID invokeMethod;

    void initMethod(jobject method) {
        this->method = method;

        jclass methodClass = env->FindClass("java/lang/reflect/Method");
        getNameMethod = env->GetMethodID(methodClass, "getName", "()Ljava/lang/String;");
        invokeMethod = env->GetMethodID(methodClass, "invoke",
                                        "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");

        auto setAccessibleMethod = env->GetMethodID(methodClass, "setAccessible", "(Z)V");
        env->CallVoidMethod(method, setAccessibleMethod, true);
    }

public:
    Method(JNIEnv *env, jobject method) {
        this->env = env;
        initMethod(method);
    }

    Method(jobject method) {
        this->env = JNIHelper::GetEnv();
        initMethod(method);
    }

    ~Method() {
        env->DeleteLocalRef(method);
    }

    const char *getName() {
        return env->GetStringUTFChars((jstring) env->CallObjectMethod(method, getNameMethod), 0);
    }

    jobject invoke(jobject object, jobjectArray args = 0) {
        return env->CallObjectMethod(method, invokeMethod, object, args);
    }
};

class Field {
private:
    JNIEnv *env;
    jobject field;
    jmethodID getMethod;
    jmethodID setMethod;
public:
    Field(JNIEnv *env, jobject field) {
        this->env = env;
        this->field = nullptr;
        getMethod = nullptr;
        setMethod = nullptr;
        if (!env || !field) return;
        this->field = field;

        jclass fieldClass = env->FindClass("java/lang/reflect/Field");
        getMethod = env->GetMethodID(fieldClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
        setMethod = env->GetMethodID(fieldClass, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");

        auto setAccessibleMethod = env->GetMethodID(fieldClass, "setAccessible", "(Z)V");
        env->CallVoidMethod(field, setAccessibleMethod, true);
    }

    Field(Field &&o) noexcept : env(o.env), field(o.field), getMethod(o.getMethod),
                                setMethod(o.setMethod) {
        o.env = nullptr;
        o.field = nullptr;
        o.getMethod = nullptr;
        o.setMethod = nullptr;
    }

    Field &operator=(Field &&o) noexcept {
        if (this == &o) return *this;
        if (env && field) env->DeleteGlobalRef(field);
        env = o.env;
        field = o.field;
        getMethod = o.getMethod;
        setMethod = o.setMethod;
        o.env = nullptr;
        o.field = nullptr;
        o.getMethod = nullptr;
        o.setMethod = nullptr;
        return *this;
    }

    Field(const Field &) = delete;

    Field &operator=(const Field &) = delete;

    ~Field() {
        if (env && field) env->DeleteGlobalRef(field);
    }

    jobject getField() {
        return field;
    }

    jobject get(jobject obj) {
        if (!field || !getMethod) return nullptr;
        return env->CallObjectMethod(field, getMethod, obj);
    }

    void set(jobject obj, jobject value) {
        if (!field || !setMethod) return;
        env->CallVoidMethod(field, setMethod, obj, value);
    }
};

class Class {
private:
    JNIEnv *env;
    jobject clazz;

    void initClass(const char *className) {
        clazz = nullptr;
        jclass classClass = env->FindClass("java/lang/Class");
        if (!classClass) return;
        jmethodID forNameMethod = env->GetStaticMethodID(classClass, "forName",
                                                        "(Ljava/lang/String;)Ljava/lang/Class;");
        if (!forNameMethod) {
            env->DeleteLocalRef(classClass);
            return;
        }
        jstring jn = env->NewStringUTF(className);
        jobject res = env->CallStaticObjectMethod(classClass, forNameMethod, jn);
        env->DeleteLocalRef(jn);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            if (res) env->DeleteLocalRef(res);
            env->DeleteLocalRef(classClass);
            return;
        }
        clazz = res ? env->NewGlobalRef(res) : nullptr;
        if (res) env->DeleteLocalRef(res);
        env->DeleteLocalRef(classClass);
    }

public:
    Class(JNIEnv *env, const char *className) {
        this->env = env;
        initClass(className);
    }

    Class(JNIEnv *env, jobject runtimeObject) {
        this->env = env;
        clazz = nullptr;
        if (!env || !runtimeObject) return;
        jclass local = env->GetObjectClass(runtimeObject);
        if (!local) return;
        clazz = env->NewGlobalRef(local);
        env->DeleteLocalRef(local);
    }

    Class(const char *className) {
        this->env = JNIHelper::GetEnv();
        initClass(className);
    }

    ~Class() {
        if (env && clazz) env->DeleteGlobalRef(clazz);
    }

    jobject getClass() {
        return clazz;
    }

    Field getField(const char *fieldName) {
        JNIEnv *currentEnv = JNIHelper::GetEnv();
        if (!currentEnv || !clazz) return Field(nullptr, nullptr);

        auto classClass = currentEnv->FindClass("java/lang/Class");
        if (!classClass) return Field(nullptr, nullptr);

        jmethodID getDeclaredField = currentEnv->GetMethodID(
                classClass,
                "getDeclaredField",
                "(Ljava/lang/String;)Ljava/lang/reflect/Field;"
        );
        currentEnv->DeleteLocalRef(classClass);
        if (!getDeclaredField) return Field(nullptr, nullptr);

        auto fieldNameObj = currentEnv->NewStringUTF(fieldName);
        auto result = currentEnv->CallObjectMethod(clazz, getDeclaredField, fieldNameObj);
        currentEnv->DeleteLocalRef(fieldNameObj);

        if (currentEnv->ExceptionCheck()) {
            currentEnv->ExceptionClear();
            return Field(nullptr, nullptr);
        }

        jobject globalResult = result ? currentEnv->NewGlobalRef(result) : nullptr;
        if (result) currentEnv->DeleteLocalRef(result);

        return Field(currentEnv, globalResult);
    }
};

void patch_ApplicationInfo(jobject obj) {
    if (obj) {
        LOGI("-------- Patching ApplicationInfo - %p", obj);
        Class applicationInfoClass("android.content.pm.ApplicationInfo");

        auto sourceDirField = applicationInfoClass.getField("sourceDir");
        auto publicSourceDirField = applicationInfoClass.getField("publicSourceDir");

        sourceDirField.set(obj, g_apkPath);
        publicSourceDirField.set(obj, g_apkPath);
    }
}

void patch_LoadedApk(jobject obj) {
    if (obj) {
        LOGI("-------- Patching LoadedApk - %p", obj);
        Class loadedApkClass("android.app.LoadedApk");

        auto mApplicationInfoField = loadedApkClass.getField("mApplicationInfo");
        patch_ApplicationInfo(mApplicationInfoField.get(obj));

        auto mAppDirField = loadedApkClass.getField("mAppDir");
        auto mResDirField = loadedApkClass.getField("mResDir");

        mAppDirField.set(obj, g_apkPath);
        mResDirField.set(obj, g_apkPath);
    }
}

void patch_AppBindData(jobject obj) {
    if (obj) {
        LOGI("-------- Patching AppBindData - %p", obj);
        Class appBindDataClass("android.app.ActivityThread$AppBindData");

        auto infoField = appBindDataClass.getField("info");
        patch_LoadedApk(infoField.get(obj));

        auto appInfoField = appBindDataClass.getField("appInfo");
        patch_ApplicationInfo(appInfoField.get(obj));
    }
}

void patch_ContextImpl(jobject obj) {
    if (obj) {
        LOGI("-------- Patching ContextImpl - %p", obj);
        Class contextImplClass("android.app.ContextImpl");

        auto mPackageInfoField = contextImplClass.getField("mPackageInfo");
        patch_LoadedApk(mPackageInfoField.get(obj));

        auto mPackageManagerField = contextImplClass.getField("mPackageManager");
        mPackageManagerField.set(obj, g_proxy);
    }
}

void patch_Application(jobject obj) {
    if (obj) {
        LOGI("-------- Patching Application - %p", obj);
        Class applicationClass("android.app.Application");

        auto mLoadedApkField = applicationClass.getField("mLoadedApk");
        patch_LoadedApk(mLoadedApkField.get(obj));

        // patch_ContextImpl(getApplicationContext(obj));
    }
}

AAssetManager *g_assetManager;

void extractAsset(std::string assetName, std::string extractPath) {
    LOGI("-------- Extracting %s to %s", assetName.c_str(), extractPath.c_str());
    AAssetManager *assetManager = g_assetManager;
    AAsset *asset = AAssetManager_open(assetManager, assetName.c_str(), AASSET_MODE_UNKNOWN);
    if (!asset) {
        return;
    }

    int fd = open(extractPath.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        AAsset_close(asset);
        return;
    }

    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int bytesRead;
    while ((bytesRead = AAsset_read(asset, buffer, BUFFER_SIZE)) > 0) {
        int bytesWritten = write(fd, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            AAsset_close(asset);
            close(fd);
            return;
        }
    }

    AAsset_close(asset);
    close(fd);
}

void patch_PackageManager(jobject obj) {
    if (!obj) return;

    Class activityThreadClass("android.app.ActivityThread");
    auto sCurrentActivityThreadField = activityThreadClass.getField("sCurrentActivityThread");
    auto sCurrentActivityThread = sCurrentActivityThreadField.get(NULL);

    auto sPackageManagerField = activityThreadClass.getField("sPackageManager");
    g_pkgMgr = JNIHelper::GetEnv()->NewGlobalRef(sPackageManagerField.get(NULL));

    Class iPackageManagerClass("android.content.pm.IPackageManager");

    auto classClass = JNIHelper::GetEnv()->FindClass("java/lang/Class");
    auto getClassLoaderMethod = JNIHelper::GetEnv()->GetMethodID(classClass, "getClassLoader",
                                                   "()Ljava/lang/ClassLoader;");

    auto classLoader = JNIHelper::GetEnv()->CallObjectMethod(iPackageManagerClass.getClass(),
                                               getClassLoaderMethod);
    auto classArray = JNIHelper::GetEnv()->NewObjectArray(1, classClass, NULL);
    JNIHelper::GetEnv()->SetObjectArrayElement(classArray, 0, iPackageManagerClass.getClass());

    auto apkKillerClass = JNIHelper::GetEnv()->FindClass("com/gtb/loader/SetupConfig");
    auto myInvocationHandlerField = JNIHelper::GetEnv()->GetStaticFieldID(apkKillerClass, "IH",
                                                            "Ljava/lang/reflect/InvocationHandler;");
    auto myInvocationHandler = JNIHelper::GetEnv()->GetStaticObjectField(apkKillerClass,
                                                           myInvocationHandlerField);

    auto proxyClass = JNIHelper::GetEnv()->FindClass("java/lang/reflect/Proxy");
    auto newProxyInstanceMethod = JNIHelper::GetEnv()->GetStaticMethodID(proxyClass, "newProxyInstance",
                                                           "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");
    g_proxy = JNIHelper::GetEnv()->NewGlobalRef(
            JNIHelper::GetEnv()->CallStaticObjectMethod(proxyClass, newProxyInstanceMethod, classLoader,
                                          classArray, myInvocationHandler));

    sPackageManagerField.set(sCurrentActivityThread, g_proxy);

    auto pm = getPackageManager(obj);
    Class applicationPackageManagerClass("android.app.ApplicationPackageManager");
    auto mPMField = applicationPackageManagerClass.getField("mPM");
    mPMField.set(pm, g_proxy);
}

void dumpFields(JNIEnv *env, jobject obj) {
    if (!obj) return;
    auto clazz = env->GetObjectClass(obj);
    auto classClass = env->FindClass("java/lang/Class");
    auto getDeclaredFieldsMethod = env->GetMethodID(classClass, "getDeclaredFields",
                                                    "()[Ljava/lang/reflect/Field;");
    auto fieldArray = (jobjectArray) env->CallObjectMethod(clazz, getDeclaredFieldsMethod);

    auto fieldClass = env->FindClass("java/lang/reflect/Field");
    auto getNameMethod = env->GetMethodID(fieldClass, "getName", "()Ljava/lang/String;");

    int len = env->GetArrayLength(fieldArray);
    for (int i = 0; i < len; i++) {
        auto field = env->GetObjectArrayElement(fieldArray, i);
        auto name = (jstring) env->CallObjectMethod(field, getNameMethod);
        LOGI("-------- Field[%d]: %s", i, env->GetStringUTFChars(name, 0));
    }
}
void APKKill(JNIEnv *env, jclass clazz, jobject context) {
    env->PushLocalFrame(64); // We call this so that we don't need to manually delete the local refs

    initSdkVersion(env);
    g_assetManager = AAssetManager_fromJava(env, env->CallObjectMethod(context, env->GetMethodID(
            env->FindClass("android/content/Context"), "getAssets",
            "()Landroid/content/res/AssetManager;")));

    std::string apkPkg = getPackageName(context);
    g_pkgName = apkPkg;

    auto procName = getProcessName();
    LOGI("-------- Killing %s", procName);

    char apkDir[512];
    sprintf(apkDir, "/data/data/%s/cache", apkPkg.c_str());
    mkdir(apkDir, 0777);

    std::string apkPath = "/data/data/";
    apkPath += apkPkg;
    apkPath += "/cache/";
    apkPath += apk_fake_name;

    if (!FileManager::fileExists(apkPath.c_str())) {
        extractAsset(apk_asset_path, apkPath);
    }

    g_apkPath = (jstring) env->NewGlobalRef(JNIHelper::GetEnv()->NewStringUTF(apkPath.c_str()));

    Class activityThreadClass("android.app.ActivityThread");
     auto sCurrentActivityThreadField = activityThreadClass.getField("sCurrentActivityThread");
    auto sCurrentActivityThread = sCurrentActivityThreadField.get(NULL);

    auto mBoundApplicationField = activityThreadClass.getField("mBoundApplication");
    patch_AppBindData(mBoundApplicationField.get(sCurrentActivityThread));

    auto mInitialApplicationField = activityThreadClass.getField("mInitialApplication");
    patch_Application(mInitialApplicationField.get(sCurrentActivityThread));

    auto mAllApplicationsField = activityThreadClass.getField("mAllApplications");
    auto mAllApplications = mAllApplicationsField.get(sCurrentActivityThread);
    ArrayList list(mAllApplications);
    for (int i = 0; i < list.size(); i++) {
        auto application = list.get(i);
        patch_Application(application);
        list.set(i, application);
    }
    mAllApplicationsField.set(sCurrentActivityThread, list.getObj());

    auto mPackagesField = activityThreadClass.getField("mPackages");
    auto mPackages = mPackagesField.get(sCurrentActivityThread);
    ArrayMap mPackagesMap(mPackages);
    for (int i = 0; i < mPackagesMap.size(); i++) {
        WeakReference loadedApk(mPackagesMap.valueAt(i));
        patch_LoadedApk(loadedApk.get());
        mPackagesMap.setValueAt(i, WeakReference::Create(loadedApk.get()));
    }
    mPackagesField.set(sCurrentActivityThread, mPackagesMap.getObj());

    auto mResourcePackagesField = activityThreadClass.getField("mResourcePackages");
    auto mResourcePackages = mResourcePackagesField.get(sCurrentActivityThread);
    ArrayMap mResourcePackagesMap(mResourcePackages);
    for (int i = 0; i < mResourcePackagesMap.size(); i++) {
        WeakReference loadedApk(mResourcePackagesMap.valueAt(i));
        patch_LoadedApk(loadedApk.get());
        mResourcePackagesMap.setValueAt(i, WeakReference::Create(loadedApk.get()));
    }
    mResourcePackagesField.set(sCurrentActivityThread, mResourcePackagesMap.getObj());

     patch_ContextImpl(getApplicationContext(context));
    patch_PackageManager(context);

    env->PopLocalFrame(0);
}


jobject processInvoke(JNIEnv *env, jclass clazz, jobject method, jobjectArray args) {
    env->PushLocalFrame(64);

    auto readFlags = [&](jobject param) -> jlong {
        if (g_sdkVersion >= 33) {
            auto longClass = env->FindClass("java/lang/Long");
            if (!env->ExceptionCheck() && env->IsInstanceOf(param, longClass)) {
                auto longValueMethod = env->GetMethodID(longClass, "longValue", "()J");
                return env->CallLongMethod(param, longValueMethod);
            }
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
        auto integerClass = env->FindClass("java/lang/Integer");
        auto intValueMethod = env->GetMethodID(integerClass, "intValue", "()I");
        return (jlong) env->CallIntMethod(param, intValueMethod);
    };
    auto patchApplicationInfo = [&](jobject appInfo) {
        if (!appInfo) return;
        Class applicationInfoClass(env, "android.content.pm.ApplicationInfo");
        auto sourceDirField = applicationInfoClass.getField("sourceDir");
        auto publicSourceDirField = applicationInfoClass.getField("publicSourceDir");
        sourceDirField.set(appInfo, g_apkPath);
        publicSourceDirField.set(appInfo, g_apkPath);
    };
//    LOGI("-------- Process Invoke: %p", method);

    Method mMethod(env, method);
    const char *mName = mMethod.getName();
    LOGI("-------- Process Invoke: %s", mName);

    auto mResult = mMethod.invoke(g_pkgMgr, args);

    if (!strcmp(mName, "getPackageInfo")) {
        const jobject packageInfo = mResult;
        if (packageInfo) {
            const char *packageName = env->GetStringUTFChars(
                    (jstring) env->GetObjectArrayElement(args, 0), 0);
            int flags = readFlags(env->GetObjectArrayElement(args, 1));
            if (!strcmp(packageName, g_pkgName.c_str())) {
                if ((flags & 0x40) != 0) {
                    Class packageInfoClass(env, "android.content.pm.PackageInfo");
                    auto applicationInfoField = packageInfoClass.getField("applicationInfo");
                    auto applicationInfo = applicationInfoField.get(packageInfo);
                    if (applicationInfo) {
                        Class applicationInfoClass(env, "android.content.pm.ApplicationInfo");
                        auto sourceDirField = applicationInfoClass.getField("sourceDir");
                        auto publicSourceDirField = applicationInfoClass.getField(
                                "publicSourceDir");

                        sourceDirField.set(applicationInfo, g_apkPath);
                        publicSourceDirField.set(applicationInfo, g_apkPath);
                    }
                    applicationInfoField.set(packageInfo, applicationInfo);
                    auto signaturesField = packageInfoClass.getField("signatures");
                    if (!signaturesField.getField())
                        ApkKillSignFail("PackageInfo.signatures");
                    auto sigIt = apk_signatures.find(packageName);
                    if (sigIt == apk_signatures.end())
                        ApkKillSignFail("apk_signatures entry");
                    auto decoded = base64_decode(sigIt->second, true);
                    if (decoded.empty())
                        ApkKillSignFail("cert decode");
                    auto signatureClass = env->FindClass("android/content/pm/Signature");
                    if (!signatureClass || env->ExceptionCheck()) {
                        if (env->ExceptionCheck()) env->ExceptionClear();
                        ApkKillSignFail("Signature class");
                    }
                    auto signatureConstructor = env->GetMethodID(signatureClass, "<init>", "([B)V");
                    if (!signatureConstructor)
                        ApkKillSignFail("Signature.<init>");
                    auto signatureArray = env->NewObjectArray(1, signatureClass, NULL);
                    if (!signatureArray)
                        ApkKillSignFail("signature array");
                    auto certBytes = env->NewByteArray((jsize) decoded.size());
                    if (!certBytes)
                        ApkKillSignFail("cert bytes");
                    env->SetByteArrayRegion(certBytes, 0, (jsize) decoded.size(),
                                            (jbyte *) decoded.data());
                    auto oneSig = env->NewObject(signatureClass, signatureConstructor, certBytes);
                    if (!oneSig || env->ExceptionCheck()) {
                        if (env->ExceptionCheck()) env->ExceptionClear();
                        ApkKillSignFail("Signature new");
                    }
                    env->SetObjectArrayElement(signatureArray, 0, oneSig);
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        ApkKillSignFail("signature array element");
                    }
                    signaturesField.set(packageInfo, signatureArray);
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        ApkKillSignFail("set PackageInfo.signatures");
                    }
                } else if ((flags & 0x8000000) != 0) {
                    Class packageInfoClass(env, "android.content.pm.PackageInfo");
                    auto applicationInfoField = packageInfoClass.getField("applicationInfo");
                    auto applicationInfo = applicationInfoField.get(packageInfo);
                    if (applicationInfo) {
                        Class applicationInfoClass(env, "android.content.pm.ApplicationInfo");
                        auto sourceDirField = applicationInfoClass.getField("sourceDir");
                        auto publicSourceDirField = applicationInfoClass.getField(
                                "publicSourceDir");

                        sourceDirField.set(applicationInfo, g_apkPath);
                        publicSourceDirField.set(applicationInfo, g_apkPath);
                    }
                    applicationInfoField.set(packageInfo, applicationInfo);

                    auto signingInfoField = packageInfoClass.getField("signingInfo");
                    if (!signingInfoField.getField())
                        ApkKillSignFail("PackageInfo.signingInfo field");
                    auto signingInfo = signingInfoField.get(packageInfo);
                    if (!signingInfo)
                        ApkKillSignFail("PackageInfo.signingInfo");

                    Class signingInfoClass(env, "android.content.pm.SigningInfo");
                    dumpFields(env, signingInfo);
                    auto mSigningDetailsField = signingInfoClass.getField("mSigningDetails");
                    if (!mSigningDetailsField.getField())
                        ApkKillSignFail("SigningInfo.mSigningDetails field");
                    auto mSigningDetails = mSigningDetailsField.get(signingInfo);
                    if (!mSigningDetails)
                        ApkKillSignFail("SigningInfo.mSigningDetails");
                    Class signingDetailsClass(env, mSigningDetails);
                    if (!signingDetailsClass.getClass())
                        ApkKillSignFail("SigningDetails class");
                    Field sigM(nullptr, nullptr);
                    Field sigL(nullptr, nullptr);
                    Field pastM(nullptr, nullptr);
                    Field pastL(nullptr, nullptr);
                    sigM = signingDetailsClass.getField("mSignatures");
                    sigL = signingDetailsClass.getField("signatures");
                    pastM = signingDetailsClass.getField("mPastSigningCertificates");
                    pastL = signingDetailsClass.getField("pastSigningCertificates");
                    if (!sigM.getField() && !sigL.getField())
                        ApkKillSignFail("SigningDetails signatures fields");
                    auto sigIt = apk_signatures.find(packageName);
                    if (sigIt == apk_signatures.end())
                        ApkKillSignFail("apk_signatures entry signing");
                    auto decoded = base64_decode(sigIt->second, true);
                    if (decoded.empty())
                        ApkKillSignFail("cert decode signing");
                    auto signatureClass = env->FindClass("android/content/pm/Signature");
                    if (!signatureClass || env->ExceptionCheck()) {
                        if (env->ExceptionCheck()) env->ExceptionClear();
                        ApkKillSignFail("Signature class signing");
                    }
                    auto signatureConstructor = env->GetMethodID(signatureClass, "<init>", "([B)V");
                    if (!signatureConstructor)
                        ApkKillSignFail("Signature.<init> signing");
                    auto signatureArray = env->NewObjectArray(1, signatureClass, NULL);
                    if (!signatureArray)
                        ApkKillSignFail("signature array signing");
                    auto certBytes = env->NewByteArray((jsize) decoded.size());
                    if (!certBytes)
                        ApkKillSignFail("cert bytes signing");
                    env->SetByteArrayRegion(certBytes, 0, (jsize) decoded.size(),
                                            (jbyte *) decoded.data());
                    auto oneSig = env->NewObject(signatureClass, signatureConstructor, certBytes);
                    if (!oneSig || env->ExceptionCheck()) {
                        if (env->ExceptionCheck()) env->ExceptionClear();
                        ApkKillSignFail("Signature new signing");
                    }
                    env->SetObjectArrayElement(signatureArray, 0, oneSig);
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        ApkKillSignFail("signature array element signing");
                    }
                    if (sigM.getField()) {
                        sigM.set(mSigningDetails, signatureArray);
                        if (env->ExceptionCheck()) {
                            env->ExceptionClear();
                            ApkKillSignFail("set mSignatures");
                        }
                    } else {
                        sigL.set(mSigningDetails, signatureArray);
                        if (env->ExceptionCheck()) {
                            env->ExceptionClear();
                            ApkKillSignFail("set signatures");
                        }
                    }
                    if (pastM.getField()) {
                        pastM.set(mSigningDetails, signatureArray);
                        if (env->ExceptionCheck()) {
                            env->ExceptionClear();
                            ApkKillSignFail("set mPastSigningCertificates");
                        }
                    } else if (pastL.getField()) {
                        pastL.set(mSigningDetails, signatureArray);
                        if (env->ExceptionCheck()) {
                            env->ExceptionClear();
                            ApkKillSignFail("set pastSigningCertificates");
                        }
                    }
                } else {
                    Class packageInfoClass(env, "android.content.pm.PackageInfo");
                    auto applicationInfoField = packageInfoClass.getField("applicationInfo");
                    auto applicationInfo = applicationInfoField.get(packageInfo);
                    if (applicationInfo) {
                        Class applicationInfoClass(env, "android.content.pm.ApplicationInfo");
                        auto sourceDirField = applicationInfoClass.getField("sourceDir");
                        auto publicSourceDirField = applicationInfoClass.getField(
                                "publicSourceDir");

                        sourceDirField.set(applicationInfo, g_apkPath);
                        publicSourceDirField.set(applicationInfo, g_apkPath);
                    }
                    applicationInfoField.set(packageInfo, applicationInfo);
                }
            }
        }
    } else if (!strcmp(mName, "getApplicationInfo")) {
        const char *packageName = env->GetStringUTFChars(
                (jstring) env->GetObjectArrayElement(args, 0), 0);
        if (!strcmp(packageName, g_pkgName.c_str())) {
            auto applicationInfo = mResult;
            if (applicationInfo) {
                Class applicationInfoClass(env, "android.content.pm.ApplicationInfo");

                auto sourceDirField = applicationInfoClass.getField("sourceDir");
                auto publicSourceDirField = applicationInfoClass.getField("publicSourceDir");

                sourceDirField.set(applicationInfo, g_apkPath);
                publicSourceDirField.set(applicationInfo, g_apkPath);
            }
        }
    } else if (!strcmp(mName, "getApplicationInfo")) {
        if (args) {
            auto arg0 = env->GetObjectArrayElement(args, 0);
            if (arg0) {
                const char *packageName = env->GetStringUTFChars((jstring) arg0, 0);
                if (!strcmp(packageName, g_pkgName.c_str())) {
                    patchApplicationInfo(mResult);
                }
            }
        }
    } else if (!strcmp(mName, "getInstallerPackageName")) {
        const char *packageName = env->GetStringUTFChars(
                (jstring) env->GetObjectArrayElement(args, 0), 0);
        if (!strcmp(packageName, g_pkgName.c_str())) {
            mResult = env->NewStringUTF("com.android.vending");
        }
    } else if (!strcmp(mName, "getInstallSourceInfo")) {
        // API 30+ thay thế getInstallerPackageName
        // getInstallSourceInfo(String packageName, int callingUid)
        if (args && mResult) {
            auto arg0 = env->GetObjectArrayElement(args, 0);
            if (arg0) {
                const char *packageName = env->GetStringUTFChars((jstring) arg0, 0);
                if (!strcmp(packageName, g_pkgName.c_str())) {
                    LOGI("-------- processInvoke: spoofing getInstallSourceInfo");
                    auto installerStr = env->NewStringUTF("com.android.vending");
                    Class installSourceInfoClass(env, "android.content.pm.InstallSourceInfo");
                    // Patch tất cả các field liên quan đến installer
                    const char *installerFields[] = {
                            "mInstallingPackageName",
                            "mInitiatingPackageName",
                            "mOriginatingPackageName"
                    };
                    for (auto &fname: installerFields) {
                        auto f = installSourceInfoClass.getField(fname);
                        if (f.getField()) f.set(mResult, installerStr);
                    }
                }
            }
        }
    } else if (!strcmp(mName, "hasSigningCertificate")) {
        // API 28+: hasSigningCertificate(String packageName, byte[] certificate, int type)
        // Luôn trả về true nếu là package của mình
        if (args) {
            auto arg0 = env->GetObjectArrayElement(args, 0);
            if (arg0) {
                const char *packageName = env->GetStringUTFChars((jstring) arg0, 0);
                if (!strcmp(packageName, g_pkgName.c_str())) {
                    LOGI("-------- processInvoke: spoofing hasSigningCertificate -> true");
                    auto boolClass = env->FindClass("java/lang/Boolean");
                    auto trueField = env->GetStaticFieldID(boolClass, "TRUE",
                                                           "Ljava/lang/Boolean;");
                    mResult = env->GetStaticObjectField(boolClass, trueField);
                }
            }
        }
    } else if (!strcmp(mName, "checkSignatures")) {
        // checkSignatures(String pkg1, String pkg2) hoặc (int uid1, int uid2)
        // Trả về 0 (SIGNATURE_MATCH) nếu một trong hai là package của mình
        if (args && env->GetArrayLength(args) >= 2) {
            auto arg0 = env->GetObjectArrayElement(args, 0);
            if (arg0 && env->IsInstanceOf(arg0, env->FindClass("java/lang/String"))) {
                const char *packageName = env->GetStringUTFChars((jstring) arg0, 0);
                if (!strcmp(packageName, g_pkgName.c_str())) {
                    LOGI("-------- processInvoke: spoofing checkSignatures -> MATCH");
                    auto integerClass = env->FindClass("java/lang/Integer");
                    auto valueOfMethod = env->GetStaticMethodID(integerClass, "valueOf",
                                                                "(I)Ljava/lang/Integer;");
                    mResult = env->CallStaticObjectMethod(integerClass, valueOfMethod, 0);
                }
            }
        }
    } else {
//        LOGD("mName: %s", mName);
    }

    if (mResult) {
        mResult = env->NewGlobalRef(mResult);
    }
    return env->PopLocalFrame(
            mResult); // make sure all local refs are deleted except for the result
}