import java.io.File

// Sau :app:assembleDebug|Release → out/dex/{variant}/ + out/lib/{module}/{variant}/{abi}/
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

            val nd = project(":native-dex").layout.buildDirectory.get().asFile
            val embedDex = File(nd, "generated/embed/classes.dex")
            check(embedDex.isFile) { "Chưa :native-dex:generateEmbeddedDex: $embedDex" }
            embedDex.copyTo(
                File(root, "out/dex/$variant/native-dex/classes.dex").also { it.parentFile.mkdirs() },
                overwrite = true,
            )
            logger.lifecycle("Copied out/dex/$variant/native-dex/classes.dex")

            fun copySo(module: String, buildDir: File, name: String) {
                val libDir =
                    File(buildDir, "intermediates/merged_native_libs/$variant/merge${cap}NativeLibs/out/lib")
                if (!libDir.isDirectory) {
                    logger.warn("Bỏ qua $module: không có $libDir (chưa merge native libs?)")
                    return
                }
                val abiDirs = libDir.listFiles()?.filter { it.isDirectory }.orEmpty()
                if (abiDirs.isEmpty()) {
                    logger.warn("Bỏ qua $module: không có thư mục ABI trong $libDir")
                    return
                }
                for (abiDir in abiDirs) {
                    val src = File(abiDir, name)
                    if (!src.isFile) {
                        logger.lifecycle("Skip ${abiDir.name}/$name")
                        continue
                    }
                    val dest = File(root, "out/lib/$module/$variant/${abiDir.name}/$name")
                    dest.parentFile.mkdirs()
                    src.copyTo(dest, overwrite = true)
                    logger.lifecycle("Copied ${dest.relativeTo(root)}")
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
