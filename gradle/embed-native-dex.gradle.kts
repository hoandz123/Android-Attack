import java.io.File
import java.util.Properties

// Apply từ :native-dex — output: build/generated/embed/
fun Project.writeEmbeddedDexHeader(dex: File, out: File) {
    val bytes = dex.readBytes()
    out.parentFile.mkdirs()
    out.bufferedWriter().use { w ->
        w.appendLine("// Generated from ${dex.path} — do not edit")
        w.appendLine("#pragma once")
        w.appendLine("#include <cstddef>")
        w.appendLine("#include <cstdint>")
        w.appendLine()
        w.appendLine("namespace embedded_dex {")
        w.appendLine("alignas(4) inline const uint8_t data[] = {")
        val chunk = 16
        for (i in bytes.indices) {
            if (i % chunk == 0) w.append("    ")
            w.append("0x")
            w.append("%02x".format(bytes[i].toInt() and 0xff))
            if (i < bytes.lastIndex) w.append(", ")
            if (i % chunk == chunk - 1 || i == bytes.lastIndex) w.appendLine()
        }
        w.appendLine("};")
        w.appendLine("inline constexpr size_t size = sizeof(data);")
        w.appendLine("} // namespace embedded_dex")
    }
}

fun Project.embedDexJar(variant: String): File {
    val cap = variant.replaceFirstChar { it.uppercase() }
    val jar = File(
        layout.buildDirectory.get().asFile,
        "intermediates/aar_main_jar/$variant/sync${cap}LibJars/classes.jar",
    )
    check(jar.isFile) { "Chưa có $jar — chạy sync${cap}LibJars" }
    return jar
}

fun Project.resolveD8(): File {
    val props = Properties()
    rootProject.file("local.properties").inputStream().use { props.load(it) }
    val sdk = props.getProperty("sdk.dir") ?: error("sdk.dir missing in local.properties")
    val toolsRoot = File(sdk, "build-tools")
    val versionDir = toolsRoot.listFiles()
        ?.filter { it.isDirectory && (File(it, "d8.bat").isFile || File(it, "d8").isFile) }
        ?.maxByOrNull { it.name }
        ?: error("Không tìm thấy build-tools d8 trong $toolsRoot")
    val win = System.getProperty("os.name").lowercase().contains("windows")
    val d8 = File(versionDir, if (win) "d8.bat" else "d8")
    check(d8.isFile) { "Không tìm thấy d8: $d8" }
    return d8
}

val generateEmbeddedDex = tasks.register("generateEmbeddedDex") {
    group = "build"
    description = "d8 classes.jar → classes.dex + embedded_dex.hpp (luôn chạy lại, không UP-TO-DATE)"
    dependsOn("compileDebugJavaWithJavac", "syncDebugLibJars")
    val outHdr = layout.buildDirectory.file("generated/embed/embedded_dex.hpp")
    val outDex = layout.buildDirectory.file("generated/embed/classes.dex")
    val d8Out = layout.buildDirectory.dir("generated/embed/d8-out")
    val jarProvider = layout.buildDirectory.file(
        "intermediates/aar_main_jar/debug/syncDebugLibJars/classes.jar",
    )
    inputs.file(jarProvider)
    outputs.files(outHdr, outDex)
    // Luôn embed dex mới nhất — không skip khi output còn timestamp cũ.
    outputs.upToDateWhen { false }
    outputs.cacheIf { false }
    doLast {
        val d8 = resolveD8()
        val jar = jarProvider.get().asFile
        check(jar.isFile) { "Chưa có $jar — chạy syncDebugLibJars" }
        val outDir = d8Out.get().asFile
        outDir.deleteRecursively()
        outDir.mkdirs()

        // Không R8 shrink — native trên TouchInputBridge được giữ vì deliverTouch gọi trực tiếp
        val proc = ProcessBuilder(
            d8.absolutePath,
            "--output",
            outDir.absolutePath,
            jar.absolutePath,
        ).redirectErrorStream(true).start()
        val d8Log = proc.inputStream.bufferedReader().readText()
        if (proc.waitFor() != 0) {
            error("d8 failed (exit ${proc.exitValue()}):\n$d8Log")
        }

        val dex = File(outDir, "classes.dex")
        check(dex.isFile) { "d8 không tạo classes.dex trong $outDir" }

        dex.copyTo(outDex.get().asFile, overwrite = true)
        writeEmbeddedDexHeader(dex, outHdr.get().asFile)
        logger.lifecycle("Embedded ${dex.length()} bytes → ${outHdr.get().asFile}")
    }
}

afterEvaluate {
    tasks.named("syncDebugLibJars") { finalizedBy(generateEmbeddedDex) }
}
