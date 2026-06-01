plugins {
    alias(libs.plugins.android.library)
}

android {
    namespace = "com.android.attack.nativecore"
    enableKotlin = false
    compileSdk {
        version = release(libs.versions.compileSdk.get().toInt()) {
            minorApiLevel = libs.versions.compileSdkMinor.get().toInt()
        }
    }

    defaultConfig {
        minSdk = libs.versions.minSdk.get().toInt()
        consumerProguardFiles("consumer-rules.pro")

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += listOf(
                    "-DANDROID_STL=c++_static",
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
                    "-DCMAKE_BUILD_TYPE=Release"
                )
            }
        }

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildFeatures {
        prefabPublishing = true
    }

    prefab {
        create("dobby") {
            headers = "src/main/cpp/dobby"
        }
        create("kitty") {
            headers = "src/main/cpp/kittymemory"
        }
        create("imgui") {
            headers = "src/main/cpp/imgui"
        }
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
    // Prebuilt libcurl + BoringSSL/nghttp2 via Prefab (not in LGL Mod Menu).
    api(libs.ndk.curl)
}

tasks.named<Delete>("clean") {
    delete(
        layout.projectDirectory.dir(".cxx"),
        layout.projectDirectory.dir(".externalNativeBuild"),
    )
}
