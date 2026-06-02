import java.io.File
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

fun runCmd(vararg cmd: String): Pair<Int, String> {
    val pb = ProcessBuilder(*cmd)
    pb.redirectErrorStream(true)
    val proc = pb.start()
    val out = proc.inputStream.bufferedReader().readText().trim()
    return proc.waitFor() to out
}

fun adbAvailable(): Boolean {
    val which = runCmd("where", "adb")
    if (which.first != 0) return false
    val state = runCmd("adb", "get-state")
    return state.first == 0 && state.second == "device"
}

tasks.register("deployZygiskDebug") {
    group = "build"
    dependsOn(tasks.named("copyDebugBuildOutputs"))

    doLast {
        val root = rootProject.projectDir
        val outRoot = File(root, "out")
        val moduleDir = File(outRoot, "magisk-module")
        val zygiskDir = File(moduleDir, "zygisk")
        moduleDir.mkdirs()
        zygiskDir.mkdirs()

        File(moduleDir, "module.prop").writeText(
            """
            id=android_attack_loader
            name=Android Attack Loader
            version=v1.0
            versionCode=1
            author=Android-Attack
            description=Zygisk loader inject libattack.so (arm64) vao com.vng.playtogether qua native bridge
            """.trimIndent() + "\n",
        )

        val metaInfDir = File(moduleDir, "META-INF/com/google/android")
        metaInfDir.mkdirs()
        File(metaInfDir, "update-binary").writeText(
            """
            #!/sbin/sh
            umask 022
            ui_print() { echo "${'$'}1"; }
            require_new_magisk() {
              ui_print "*******************************"
              ui_print " Vui long cai dat Magisk tu v20.4+ tro len! "
              ui_print "*******************************"
              exit 1
            }
            OUTFD=${'$'}2
            ZIPFILE=${'$'}3
            mount /data 2>/dev/null
            [ -f /data/adb/magisk/util_functions.sh ] || require_new_magisk
            . /data/adb/magisk/util_functions.sh
            [ ${'$'}MAGISK_VER_CODE -lt 20400 ] && require_new_magisk
            install_module
            exit 0
            """.trimIndent() + "\n",
        )
        File(metaInfDir, "updater-script").writeText("#MAGISK\n")

        val abis = listOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
        for (abi in abis) {
            val src = File(outRoot, "lib/loader/debug/$abi/libloader.so")
            if (!src.isFile) {
                logger.lifecycle("Skip zygisk/$abi.so (chua co ${src.relativeTo(root)})")
                continue
            }
            val dest = File(zygiskDir, "$abi.so")
            src.copyTo(dest, overwrite = true)
            logger.lifecycle("Copied ${dest.relativeTo(root)}")
        }

        val zipFile = File(outRoot, "AndroidAttackLoader-zygisk.zip")
        ZipOutputStream(zipFile.outputStream()).use { zos ->
            moduleDir.walkTopDown().filter { it.isFile }.forEach { file ->
                val entryName = file.relativeTo(moduleDir).invariantSeparatorsPath
                zos.putNextEntry(ZipEntry(entryName))
                file.inputStream().copyTo(zos)
                zos.closeEntry()
            }
        }
        logger.lifecycle("Packed ${zipFile.relativeTo(root)}")

        if (!adbAvailable()) {
            logger.warn("adb khong san sang hoac khong co thiet bi — bo qua push payload / restart game")
            return@doLast
        }

        val payload = File(outRoot, "lib/attack/debug/arm64-v8a/libattack.so")
        if (!payload.isFile) {
            logger.warn("Khong co payload arm64: $payload")
            return@doLast
        }

        val targetPkg = "com.vng.playtogether"
        val targetPath = "/data/user/0/$targetPkg/libattack.so"

        try {
            val push = runCmd("adb", "push", payload.absolutePath, "/data/local/tmp/libattack.so")
            if (push.first != 0) {
                logger.warn("adb push that bai: ${push.second}")
                return@doLast
            }
            val deploy = runCmd(
                "adb", "shell", "su", "-c",
                "cp /data/local/tmp/libattack.so $targetPath || exit 1; " +
                    "chmod 644 $targetPath 2>/dev/null; " +
                    "[ \$(getenforce) = Enforcing ] && chcon u:object_r:app_data_file:s0 $targetPath 2>/dev/null; true",
            )
            if (deploy.first != 0) logger.warn("su cp that bai: ${deploy.second}")
            else logger.lifecycle("Da copy payload -> $targetPath")
            runCmd("adb", "shell", "am", "force-stop", targetPkg)
            val launch = runCmd("adb", "shell", "monkey", "-p", targetPkg, "-c", "android.intent.category.LAUNCHER", "1")
            if (launch.first != 0) logger.warn("monkey launch that bai: ${launch.second}") else logger.lifecycle("Da mo lai $targetPkg")
        } catch (e: Exception) {
            logger.warn("deploy adb loi (bo qua): ${e.message}")
        }
    }
}

gradle.projectsEvaluated {
    project(":app").tasks.findByName("assembleDebug")?.finalizedBy(tasks.named("deployZygiskDebug"))
}
