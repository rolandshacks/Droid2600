plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "org.codewiz.droid2600"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.codewiz.droid2600"
        minSdk = 24
        targetSdk = 35
        versionCode = 3
        versionName = "1.2.0"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
            signingConfig = signingConfigs.getByName("debug")
        }
        debug {
            isMinifyEnabled = false
        }
    }

    applicationVariants.all {
        val variant = this

        variant.outputs
            .map {
                it as com.android.build.gradle.internal.api.BaseVariantOutputImpl
            }
            .forEach { output ->
                val outputFileName = "${applicationId}-${variant.versionName}.apk"
                println("OutputFileName: $outputFileName")
                output.outputFileName = outputFileName
            }
    }

    externalNativeBuild {
        cmake {
            path = file("CMakeLists.txt")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}