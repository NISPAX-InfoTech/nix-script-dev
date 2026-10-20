plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}
android {
    namespace = "in.nispax.nix"
    compileSdk = 35
    ndkVersion = "27.2.12479018"
    defaultConfig {
        applicationId = "in.nispax.nix"
        minSdk = 24
        targetSdk = 35
        versionCode = 10001
        versionName = "1.0.0-beta.1"
        externalNativeBuild { cmake { cppFlags += "-std=c++17"; arguments += "-DNIX_ANDROID=1" } }
    }
    signingConfigs {
        create("release") {
            val storePath = System.getenv("ANDROID_KEYSTORE_PATH")
            if (!storePath.isNullOrBlank()) storeFile = file(storePath)
            storePassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
            keyAlias = System.getenv("ANDROID_KEY_ALIAS")
            keyPassword = System.getenv("ANDROID_KEY_PASSWORD")
        }
    }
    buildTypes {
        debug { versionNameSuffix = "-debug" }
        release {
            isMinifyEnabled = false
            val signed = !System.getenv("ANDROID_KEYSTORE_PATH").isNullOrBlank()
                && !System.getenv("ANDROID_KEYSTORE_PASSWORD").isNullOrBlank()
                && !System.getenv("ANDROID_KEY_ALIAS").isNullOrBlank()
                && !System.getenv("ANDROID_KEY_PASSWORD").isNullOrBlank()
            if (signed) signingConfig = signingConfigs.getByName("release")
        }
    }
    externalNativeBuild { cmake { path = file("src/main/cpp/CMakeLists.txt"); version = "3.22.1" } }
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
    kotlinOptions { jvmTarget = "17" }
}
