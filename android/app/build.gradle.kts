/*
 * build.gradle.kts
 *
 * Copyright (C) 2026, Charles Chiou
 */

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)
}

android {
    namespace = "com.selfso.octi"
    compileSdk = 35

    val readmeFile = rootProject.projectDir.resolve("../README.md")
    val versionRegex = Regex("""\*\*Version:\s*([0-9]+)\.([0-9]+)\.([0-9]+)\*\*""")
    val versionMatch = versionRegex.find(if (readmeFile.exists()) readmeFile.readText() else "")
        ?: error("Could not extract **Version: x.y.z** from ${readmeFile.absolutePath}")
    val (projMajor, projMinor, projPatch) = Triple(
        versionMatch.groupValues[1],
        versionMatch.groupValues[2],
        versionMatch.groupValues[3]
    )
    val projVersionName = "$projMajor.$projMinor.$projPatch"
    val projVersionCode = projMajor.toInt() * 10000 + projMinor.toInt() * 100 + projPatch.toInt()

    defaultConfig {
        applicationId = "com.selfso.octi"
        minSdk = 24
        targetSdk = 35
        versionCode = projVersionCode
        versionName = projVersionName

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
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
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
    kotlinOptions {
        jvmTarget = "21"
    }
    buildFeatures {
        compose = true
    }
    testOptions {
        unitTests.isReturnDefaultValues = true
    }
}

dependencies {
    implementation(project(":selfupdate"))
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.lifecycle.runtime.ktx)
    implementation(libs.androidx.activity.compose)
    implementation(platform(libs.androidx.compose.bom))
    implementation(libs.androidx.ui)
    implementation(libs.androidx.ui.graphics)
    implementation(libs.androidx.ui.tooling.preview)
    implementation(libs.androidx.material3)
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")

    testImplementation(libs.junit)
    testImplementation("org.json:json:20240303")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.8.1")
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)
    androidTestImplementation(platform(libs.androidx.compose.bom))
    androidTestImplementation(libs.androidx.ui.test.junit4)

    debugImplementation(libs.androidx.ui.tooling)
    debugImplementation(libs.androidx.ui.test.manifest)
}
