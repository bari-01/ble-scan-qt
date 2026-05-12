pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}
plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "qshare"
include(":app")

includeBuild("/home/a/cpp/simpleble/simpleble/src/backends/android/bridge") {
    dependencySubstitution {
        substitute(module("org.simpleble.android.bridge:simpleble-bridge")).using(project(":"))
    }
}
