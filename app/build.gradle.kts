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
                    "-DNATIVE_CORE_CPP=${rootProject.projectDir}/native-core/src/main/cpp"
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
}

dependencies {
    implementation(project(":loader"))
    implementation(project(":native-core"))
}

// Avoid CMake reconfigure during clean after :native-core:clean drops Prefab outputs.
tasks.matching { it.name.startsWith("externalNativeBuildClean") }.configureEach {
    enabled = false
}

tasks.named<Delete>("clean") {
    delete(
        layout.projectDirectory.dir(".cxx"),
        layout.projectDirectory.dir(".externalNativeBuild"),
    )
}
