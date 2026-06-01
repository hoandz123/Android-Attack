plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.android.attack"
    enableKotlin = false
    compileSdk {
        version = release(libs.versions.compileSdk.get().toInt()) {
            minorApiLevel = libs.versions.compileSdkMinor.get().toInt()
        }
    }

    defaultConfig {
        applicationId = "com.android.attack"
        minSdk = libs.versions.minSdk.get().toInt()
        targetSdk = libs.versions.targetSdk.get().toInt()
        versionCode = 1
        versionName = "1.0"
        multiDexEnabled = false

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += listOf(
                    "-DANDROID_STL=c++_static",
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DNATIVE_CORE_CPP=${rootProject.projectDir}/native-core/src/main/cpp",
                    "-DNATIVE_CORE_PREFAB=${rootProject.projectDir}/native-core/build/intermediates/prefab_package/release/prefab",
                    "-DMOD_UI_PREFAB=${rootProject.projectDir}/mod-ui/build/intermediates/prefab_package/release/prefab",
                    "-DEMBEDDED_DEX_DIR=${rootProject.projectDir}/native-dex/build/generated/embed".replace('\\', '/'),
                )
            }
        }

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildFeatures {
        prefab = true
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = libs.versions.cmake.get()
        }
    }

    ndkVersion = libs.versions.ndk.get()

    packaging {
        jniLibs {
            // Extract .so to nativeLibraryDir so loader dlopen(path) works on device/OEM.
            useLegacyPackaging = true
        }
    }
}

dependencies {
    implementation(project(":loader"))
    implementation(project(":native-core"))
    implementation(project(":mod-ui"))
}

// Avoid CMake reconfigure during clean after :native-core:clean drops Prefab outputs.
tasks.matching { it.name.startsWith("externalNativeBuildClean") }.configureEach {
    enabled = false
}

afterEvaluate {
    val embed = ":native-dex:generateEmbeddedDex"
    tasks.named("preBuild") { dependsOn(embed) }
    tasks.matching {
        it.name.startsWith("configureCMake") || it.name.contains("buildCMake")
    }.configureEach { dependsOn(embed) }
}

tasks.named<Delete>("clean") {
    delete(
        layout.projectDirectory.dir(".cxx"),
        layout.projectDirectory.dir(".externalNativeBuild"),
        rootProject.layout.projectDirectory.dir("out"),
    )
}
