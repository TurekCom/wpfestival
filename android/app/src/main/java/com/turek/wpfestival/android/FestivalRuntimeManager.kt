package com.turek.wpfestival.android

import android.content.Context
import android.util.Log
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.Charset
import java.util.concurrent.TimeUnit
import kotlin.math.pow
import kotlin.math.roundToInt

data class SynthResult(
    val sampleRate: Int,
    val pcm: ByteArray,
)

object FestivalRuntimeManager {
    private const val TAG = "WpFestivalRuntime"
    private const val RUNTIME_VERSION = 1
    private const val DEFAULT_PITCH_MEAN = 105
    private const val DEFAULT_PITCH_STD = 14
    private const val DEFAULT_MODEL_PITCH_MEAN = 170
    private const val DEFAULT_MODEL_PITCH_STD = 34
    private const val EXECUTABLE_NAME = "libwpfestival_exec.so"
    private val WINDOWS_1250: Charset = Charset.forName("windows-1250")
    private val installLock = Any()

    @Volatile
    private var activeProcess: Process? = null

    fun ensureRuntimeInstalled(context: Context): File {
        synchronized(installLock) {
            val root = File(context.filesDir, "wp_festival_runtime")
            val runtimeDir = File(root, "wp_runtime_lib")
            val stampFile = File(root, ".runtime_version")
            val currentVersion = if (stampFile.exists()) stampFile.readText().trim() else ""
            if (currentVersion == RUNTIME_VERSION.toString() && File(runtimeDir, "festival.scm").exists()) {
                Log.i(TAG, "Runtime already installed at ${runtimeDir.absolutePath}")
                return runtimeDir
            }

            root.deleteRecursively()
            runtimeDir.mkdirs()
            copyAssetTree(context, "runtime/common/wp_runtime_lib", runtimeDir)
            stampFile.writeText(RUNTIME_VERSION.toString())
            Log.i(TAG, "Runtime installed at ${runtimeDir.absolutePath}")
            return runtimeDir
        }
    }

    fun runtimeStatus(context: Context): String {
        val runtimeDir = ensureRuntimeInstalled(context)
        val executable = festivalExecutable(context)
        val abi = currentAbi(context)
        return if (executable.exists()) {
            "${abi ?: "unknown"}, runtime ${runtimeDir.name}"
        } else {
            "Brak natywnego backendu dla bieżącego ABI"
        }
    }

    fun stopActiveSynthesis() {
        activeProcess?.destroy()
        activeProcess?.destroyForcibly()
        activeProcess = null
    }

    fun synthesize(
        context: Context,
        text: String,
        voice: WpFestivalVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
    ): SynthResult {
        val runtimeDir = ensureRuntimeInstalled(context)
        val executable = festivalExecutable(context)
        require(executable.exists()) { "Missing festival executable for current ABI" }

        val workDir = File(context.cacheDir, "wp_festival_synth").apply { mkdirs() }
        val scriptFile = File.createTempFile("wpf_", ".scm", workDir)
        val wavFile = File(workDir, scriptFile.nameWithoutExtension + ".wav")
        val logFile = File(workDir, scriptFile.nameWithoutExtension + ".log")

        try {
            writeSchemeScript(scriptFile, wavFile, text, voice, ratePercent, pitchPercent, volumePercent)
            Log.i(
                TAG,
                "Launching festival exec=${executable.absolutePath} abi=${currentAbi(context)} voice=${voice.name}",
            )

            val processBuilder = ProcessBuilder(
                executable.absolutePath,
                "--libdir",
                runtimeDir.absolutePath,
                "-b",
                scriptFile.absolutePath,
            )
            processBuilder.directory(File(context.applicationInfo.nativeLibraryDir))
            processBuilder.redirectErrorStream(true)
            processBuilder.redirectOutput(logFile)
            val environment = processBuilder.environment()
            environment["LD_LIBRARY_PATH"] = context.applicationInfo.nativeLibraryDir
            environment["HOME"] = context.filesDir.absolutePath
            environment["TMPDIR"] = context.cacheDir.absolutePath

            val process = processBuilder.start()
            activeProcess = process
            val finished = process.waitFor(60, TimeUnit.SECONDS)
            activeProcess = null
            if (!finished) {
                process.destroyForcibly()
                Log.e(TAG, "Festival timeout")
                throw IllegalStateException("Festival timeout")
            }
            if (process.exitValue() != 0) {
                val logTail = runCatching { logFile.readText() }.getOrDefault("")
                Log.e(TAG, "Festival exited with ${process.exitValue()}: $logTail")
                throw IllegalStateException("Festival exited with ${process.exitValue()}")
            }
            require(wavFile.exists()) { "Festival did not create wav output" }
            Log.i(TAG, "Festival wrote wav ${wavFile.length()} bytes")
            return parseWavePcm(wavFile.readBytes())
        } finally {
            activeProcess = null
            scriptFile.delete()
            wavFile.delete()
            logFile.delete()
        }
    }

    private fun festivalExecutable(context: Context): File =
        File(context.applicationInfo.nativeLibraryDir, EXECUTABLE_NAME)

    private fun currentAbi(context: Context): String? =
        context.applicationInfo.nativeLibraryDir
            ?.takeIf { it.isNotBlank() }
            ?.let { File(it).name }

    private fun copyAssetTree(context: Context, assetPath: String, destination: File) {
        val children = context.assets.list(assetPath).orEmpty()
        if (children.isEmpty()) {
            destination.parentFile?.mkdirs()
            context.assets.open(assetPath).use { input ->
                destination.outputStream().use { output -> input.copyTo(output) }
            }
            return
        }

        destination.mkdirs()
        children.forEach { child ->
            copyAssetTree(context, "$assetPath/$child", File(destination, child))
        }
    }

    private fun writeSchemeScript(
        scriptFile: File,
        wavFile: File,
        text: String,
        voice: WpFestivalVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
    ) {
        val preset = buildEffectivePreset(voice, ratePercent, pitchPercent, volumePercent)
        val wavPath = wavFile.absolutePath.replace('\\', '/')
        val escapedText = escapeSchemeString(normalizeText(text))
        val script = buildString {
            appendLine("(voice_wp_pl_m1_diphone)")
            appendLine("(set! int_lr_params '((target_f0_mean ${preset.pitchMean}) (target_f0_std ${preset.pitchStd}) (model_f0_mean $DEFAULT_MODEL_PITCH_MEAN) (model_f0_std $DEFAULT_MODEL_PITCH_STD)))")
            appendLine("(set! wp_android_pitch_mean ${preset.pitchMean})")
            appendLine("(set! wp_android_pitch_std_scale ${preset.pitchStd.toDouble() / DEFAULT_PITCH_STD.toDouble()})")
            appendLine("(define (wp_android_pitch_target_wrap utt syl)")
            appendLine("  (mapcar")
            appendLine("    (lambda (target)")
            appendLine("      (let ((scaled (+ wp_android_pitch_mean (* (- (cadr target) $DEFAULT_PITCH_MEAN) wp_android_pitch_std_scale))))")
            appendLine("        (list")
            appendLine("          (car target)")
            appendLine("          (if (< scaled 55) 55 (if (> scaled 220) 220 scaled)))))")
            appendLine("    (wp_pl_m1_targ_func1 utt syl)))")
            appendLine("(set! int_general_params (list (list 'targ_func wp_android_pitch_target_wrap)))")
            appendLine("(set! Set_Duration_Stretch ${rateAdjustToDurationStretch(preset.rateAdjust)})")
            appendLine("(Parameter.set 'Duration_Stretch Set_Duration_Stretch)")
            appendLine("(set! tts_volume ${siteVolumeToTtsVolume(preset.volumePercent)})")
            appendLine("(set! utt1 (Utterance Text \"$escapedText\"))")
            appendLine("(utt.synth utt1)")
            appendLine("(utt.wave.rescale utt1 tts_volume nil)")
            appendLine("(utt.save.wave utt1 \"$wavPath\" 'riff)")
            appendLine("(exit)")
        }
        scriptFile.writeBytes(script.toByteArray(WINDOWS_1250))
    }

    private data class EffectivePreset(
        val rateAdjust: Int,
        val volumePercent: Int,
        val pitchMean: Int,
        val pitchStd: Int,
    )

    private fun buildEffectivePreset(
        voice: WpFestivalVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
    ): EffectivePreset {
        val safeRatePercent = clamp(ratePercent, WpFestivalRanges.RATE_MIN, WpFestivalRanges.RATE_MAX)
        val safePitchPercent = clamp(pitchPercent, WpFestivalRanges.PITCH_MIN, WpFestivalRanges.PITCH_MAX)
        val safeVolumePercent = clamp(volumePercent, WpFestivalRanges.VOLUME_MIN, WpFestivalRanges.VOLUME_MAX)
        val rateAdjust = clamp(voice.preset.baseRateAdjust + uiPercentToRateAdjust(safeRatePercent), -15, 15)
        val pitchMean = clamp(voice.preset.pitchMean + uiPercentToPitchMeanDelta(safePitchPercent), 60, 210)
        val finalVolume = clamp(((safeVolumePercent * voice.preset.baseVolumePercent) / 100.0).roundToInt(), 0, 200)
        return EffectivePreset(rateAdjust, finalVolume, pitchMean, clamp(voice.preset.pitchStd, 6, 40))
    }

    private fun normalizeText(text: String): String =
        text.replace(Regex("\\s+"), " ").trim()

    private fun escapeSchemeString(text: String): String {
        val out = StringBuilder(text.length + 16)
        text.forEach { ch ->
            when (ch) {
                '\\' -> out.append("\\\\")
                '"' -> out.append("\\\"")
                '\r', '\n', '\t' -> out.append(' ')
                else -> out.append(ch)
            }
        }
        return out.toString()
    }

    private fun uiPercentToRateAdjust(percent: Int): Int =
        (((percent - WpFestivalRanges.RATE_NEUTRAL).toDouble() / 100.0) * 15.0)
            .roundToInt()
            .coerceIn(-15, 15)

    private fun uiPercentToPitchMeanDelta(percent: Int): Int =
        (((percent - WpFestivalRanges.PITCH_NEUTRAL).toDouble() / 100.0) * 60.0).roundToInt()

    private fun rateAdjustToDurationStretch(rateAdjust: Int): Double =
        (2.0.pow(-clamp(rateAdjust, -15, 15).toDouble() / 10.0)).coerceIn(0.30, 3.00)

    private fun siteVolumeToTtsVolume(volumePercent: Int): Double =
        ((clamp(volumePercent, WpFestivalRanges.VOLUME_MIN, WpFestivalRanges.VOLUME_MAX).toDouble() / 100.0) * 2.0)
            .coerceIn(0.0, 4.0)

    private fun clamp(value: Int, min: Int, max: Int): Int =
        value.coerceIn(min, max)

    private fun parseWavePcm(bytes: ByteArray): SynthResult {
        require(bytes.size >= 44) { "Invalid wav header" }
        require(String(bytes.copyOfRange(0, 4)) == "RIFF") { "Missing RIFF" }
        require(String(bytes.copyOfRange(8, 12)) == "WAVE") { "Missing WAVE" }

        var offset = 12
        var sampleRate = 0
        var channels = 0
        var bitsPerSample = 0
        var pcm: ByteArray? = null

        while (offset + 8 <= bytes.size) {
            val id = String(bytes, offset, 4)
            val size = littleEndianInt(bytes, offset + 4)
            val dataOffset = offset + 8
            val nextOffset = dataOffset + size + (size and 1)
            require(nextOffset <= bytes.size) { "Corrupt wav chunk" }

            when (id) {
                "fmt " -> {
                    channels = littleEndianShort(bytes, dataOffset + 2).toInt()
                    sampleRate = littleEndianInt(bytes, dataOffset + 4)
                    bitsPerSample = littleEndianShort(bytes, dataOffset + 14).toInt()
                }
                "data" -> pcm = bytes.copyOfRange(dataOffset, dataOffset + size)
            }

            offset = nextOffset
        }

        require(channels == 1) { "Only mono output is supported" }
        require(bitsPerSample == 16) { "Only PCM 16-bit output is supported" }
        require(sampleRate > 0) { "Missing sample rate" }
        return SynthResult(sampleRate = sampleRate, pcm = pcm ?: ByteArray(0))
    }

    private fun littleEndianInt(bytes: ByteArray, offset: Int): Int =
        ByteBuffer.wrap(bytes, offset, 4).order(ByteOrder.LITTLE_ENDIAN).int

    private fun littleEndianShort(bytes: ByteArray, offset: Int): Short =
        ByteBuffer.wrap(bytes, offset, 2).order(ByteOrder.LITTLE_ENDIAN).short
}
