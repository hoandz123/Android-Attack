plugins {
    alias(libs.plugins.android.library)
}

// Chỉ Java — DEX được embed vào libattack.so, không đưa vào APK qua :app.
android {
    namespace = "com.android.attack.nativedex"
    enableKotlin = false
    compileSdk {
        version = release(libs.versions.compileSdk.get().toInt()) {
            minorApiLevel = libs.versions.compileSdkMinor.get().toInt()
        }
    }

    defaultConfig {
        minSdk = libs.versions.minSdk.get().toInt()
        consumerProguardFiles("consumer-rules.pro")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}

apply(from = rootProject.file("gradle/embed-native-dex.gradle.kts"))
