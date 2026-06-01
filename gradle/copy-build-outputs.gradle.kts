import java.io.File

// Sau :app:assembleDebug|Release → out/dex/{variant}/ + out/lib/{module}/{variant}/{abi}/
val abis = listOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")

fun registerCopy(variant: String) {
    val cap = variant.replaceFirstChar { it.uppercase() }
    tasks.register("copy${cap}BuildOutputs") {
        group = "build"
        doLast {
            val root = rootProject.projectDir
            val app = project(":app").layout.buildDirectory.get().asFile
            val loader = project(":loader").layout.buildDirectory.get().asFile

            val dex = File(app, "intermediates/dex/$variant/mergeDex$cap/classes.dex")
            check(dex.isFile) { "Chưa build dex: $dex" }
            dex.copyTo(File(root, "out/dex/$variant/classes.dex").also { it.parentFile.mkdirs() }, overwrite = true)

            fun copySo(module: String, buildDir: File, name: String) {
                val libDir = File(buildDir, "intermediates/merged_native_libs/$variant/merge${cap}NativeLibs/out/lib")
                for (abi in abis) {
                    val src = File(libDir, "$abi/$name")
                    check(src.isFile) { "Thiếu $name ($abi): $src" }
                    src.copyTo(File(root, "out/lib/$module/$variant/$abi/$name").also { it.parentFile.mkdirs() }, overwrite = true)
                }
            }
            copySo("attack", app, "libattack.so")
            copySo("loader", loader, "libloader.so")
        }
    }
}

registerCopy("debug")
registerCopy("release")

gradle.projectsEvaluated {
    project(":app").tasks.findByName("assembleDebug")?.finalizedBy(tasks.named("copyDebugBuildOutputs"))
    project(":app").tasks.findByName("assembleRelease")?.finalizedBy(tasks.named("copyReleaseBuildOutputs"))
}
