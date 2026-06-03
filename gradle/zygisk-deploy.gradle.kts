import java.io.File
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream
import org.gradle.api.logging.Logger

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

/** Windows hay khoá .so đang mở — copyTo(overwrite) fail ngẫu nhiên. */
fun copyFileRobust(src: File, dest: File, logger: Logger, maxAttempts: Int = 8): Boolean {
    dest.parentFile?.mkdirs()
    val tmp = File(dest.parentFile, "${dest.name}.deploytmp")
    var last: Exception? = null
    repeat(maxAttempts) { attempt ->
        try {
            Files.copy(src.toPath(), tmp.toPath(), StandardCopyOption.REPLACE_EXISTING)
            deleteQuiet(dest)
            try {
                Files.move(
                    tmp.toPath(),
                    dest.toPath(),
                    StandardCopyOption.REPLACE_EXISTING,
                    StandardCopyOption.ATOMIC_MOVE,
                )
            } catch (_: Exception) {
                Files.move(tmp.toPath(), dest.toPath(), StandardCopyOption.REPLACE_EXISTING)
            }
            if (tmp.exists()) tmp.delete()
            return true
        } catch (e: Exception) {
            last = e
            if (tmp.exists()) tmp.delete()
            if (attempt + 1 < maxAttempts) Thread.sleep(50L * (attempt + 1))
        }
    }
    logger.warn("Khong ghi duoc ${dest.name} sau $maxAttempts lan (${last?.message})")
    return false
}

fun deleteQuiet(file: File): Boolean {
    if (!file.exists()) return true
    return try {
        Files.delete(file.toPath())
        true
    } catch (_: Exception) {
        file.delete()
    }
}

fun org.gradle.api.Project.gradleProp(key: String, default: String): String =
    (findProperty(key) as String?)?.trim()?.takeIf { it.isNotEmpty() } ?: default

fun org.gradle.api.Project.ndkAbis(): List<String> =
    gradleProp("attack.ndkAbis", "arm64-v8a")
        .split(",")
        .map { it.trim() }
        .filter { it.isNotEmpty() }

tasks.register("deployZygiskDebug") {
    group = "build"
    dependsOn(tasks.named("copyDebugBuildOutputs"))

    doLast {
        val root = rootProject.projectDir
        val targetPkg = project.gradleProp("attack.pushTarget", "com.vng.playtogether")
        val moduleDesc = project.gradleProp(
            "attack.magiskModule.description",
            "Zygisk loader inject libattack.so (arm64); adb payload -> {package}",
        ).replace("{package}", targetPkg)
        val outRoot = File(root, "out")
        val moduleDir = File(outRoot, "magisk-module")
        val zygiskDir = File(moduleDir, "zygisk")
        moduleDir.mkdirs()
        zygiskDir.mkdirs()

        File(moduleDir, "module.prop").writeText(
            """
            id=${project.gradleProp("attack.magiskModule.id", "android_attack_loader")}
            name=${project.gradleProp("attack.magiskModule.name", "Android Attack Loader")}
            version=${project.gradleProp("attack.magiskModule.version", "v1.0")}
            versionCode=${project.gradleProp("attack.magiskModule.versionCode", "1")}
            author=${project.gradleProp("attack.magiskModule.author", "Android-Attack")}
            description=$moduleDesc
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

        val abis = project.ndkAbis()
        logger.lifecycle("Zygisk ABI (attack.ndkAbis): ${abis.joinToString()}")

        // Xóa .so ABI không còn build — tránh zip/deploy nhầm bản cũ (out/lib có thể còn từ build đủ ABI trước đó)
        val keepNames = abis.map { "$it.so" }.toSet()
        zygiskDir.listFiles()?.filter { it.isFile && it.name.endsWith(".so") && it.name !in keepNames }
            ?.forEach { stale ->
                if (deleteQuiet(stale)) logger.lifecycle("Removed stale ${stale.relativeTo(root)}")
            }

        for (abi in abis) {
            val src = File(outRoot, "lib/loader/debug/$abi/libloader.so")
            if (!src.isFile) {
                logger.lifecycle("Skip zygisk/$abi.so (chua co ${src.relativeTo(root)})")
                continue
            }
            val dest = File(zygiskDir, "$abi.so")
            if (!copyFileRobust(src, dest, logger)) {
                logger.warn(
                    "Skip zygisk/$abi.so (Windows khoa file? dong Explorer/antivirus tren out/magisk-module, hoac xoa thu muc zygisk roi build lai)",
                )
                continue
            }
            logger.lifecycle("Copied ${dest.relativeTo(root)}")
        }

        val zipFile = File(outRoot, "AndroidAttackLoader-zygisk.zip")
        val zipTmp = File(outRoot, "AndroidAttackLoader-zygisk.zip.deploytmp")
        deleteQuiet(zipFile)
        ZipOutputStream(zipTmp.outputStream()).use { zos ->
            moduleDir.walkTopDown().filter { it.isFile }.forEach { file ->
                val entryName = file.relativeTo(moduleDir).invariantSeparatorsPath
                zos.putNextEntry(ZipEntry(entryName))
                file.inputStream().copyTo(zos)
                zos.closeEntry()
            }
        }
        if (!copyFileRobust(zipTmp, zipFile, logger)) {
            logger.warn("Khong ghi duoc zip Zygisk — bo qua adb deploy")
            return@doLast
        }
        zipTmp.delete()
        logger.lifecycle("Packed ${zipFile.relativeTo(root)}")

        if (!adbAvailable()) {
            logger.warn("adb khong san sang hoac khong co thiet bi — bo qua push payload / restart game")
            return@doLast
        }

        val pushAbi = project.gradleProp("attack.pushAbi", "arm64-v8a")
        val payload = File(outRoot, "lib/attack/debug/$pushAbi/libattack.so")
        if (!payload.isFile) {
            logger.warn("Khong co payload ($pushAbi): $payload")
            return@doLast
        }
        logger.lifecycle("adb payload ABI: $pushAbi")

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
