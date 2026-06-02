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
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
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
        create("lgl") {
            // Export cpp root so consumers can #include <Includes/Logger.h> (LGL style).
            headers = "src/main/cpp"
        }
        create("dobby") {
            headers = "src/main/cpp/dobby"
        }
        create("kitty") {
            headers = "src/main/cpp/kittymemory"
        }
        create("imgui") {
            headers = "src/main/cpp/imgui"
        }
        create("curl") {
            headers = "src/main/cpp/curl/include"
        }
        create("httpclient") {
            headers = "src/main/cpp/HttpClient"
        }
        create("filemanager") {
            headers = "src/main/cpp/FileManager"
        }
        create("jnihelper") {
            headers = "src/main/cpp"
        }
        create("dexloader") {
            headers = "src/main/cpp"
        }
        create("activitytracker") {
            headers = "src/main/cpp"
        }
        create("tools") {
            headers = "src/main/cpp/Tools"
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = libs.versions.cmake.get()
        }
    }

    ndkVersion = libs.versions.ndk.get()

    buildTypes {
        debug {
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Debug"
                }
            }
        }
        release {
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }
    }
}

tasks.named<Delete>("clean") {
    delete(
        layout.projectDirectory.dir(".cxx"),
        layout.projectDirectory.dir(".externalNativeBuild"),
    )
}
