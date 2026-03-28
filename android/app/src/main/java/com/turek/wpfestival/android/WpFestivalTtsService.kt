package com.turek.wpfestival.android

import android.media.AudioFormat
import android.os.Bundle
import android.speech.tts.SynthesisCallback
import android.speech.tts.SynthesisRequest
import android.speech.tts.TextToSpeech
import android.speech.tts.TextToSpeechService
import android.speech.tts.Voice
import android.util.Log
import java.nio.charset.Charset
import kotlin.math.roundToInt

class WpFestivalTtsService : TextToSpeechService() {
    companion object {
        private const val TAG = "WpFestivalTts"
        private val WINDOWS_1250: Charset = Charset.forName("windows-1250")
    }

    @Volatile
    private var stopRequested = false

    override fun onGetLanguage(): Array<String> = arrayOf(
        WpFestivalEngine.iso3Language(),
        WpFestivalEngine.iso3Country(),
        "",
    )

    override fun onIsLanguageAvailable(language: String, country: String, variant: String): Int {
        return if (WpFestivalEngine.matches(language, country)) {
            TextToSpeech.LANG_COUNTRY_AVAILABLE
        } else {
            TextToSpeech.LANG_NOT_SUPPORTED
        }
    }

    override fun onLoadLanguage(language: String, country: String, variant: String): Int =
        onIsLanguageAvailable(language, country, variant)

    override fun onGetDefaultVoiceNameFor(language: String, country: String, variant: String): String? {
        return if (WpFestivalEngine.matches(language, country)) {
            WpFestivalPreferences.load(this).voiceName
        } else {
            null
        }
    }

    override fun onGetVoices(): MutableList<Voice> =
        WpFestivalEngine.voices.map { WpFestivalEngine.toAndroidVoice(it) }.toMutableList()

    override fun onStop() {
        stopRequested = true
        Log.i(TAG, "Stop requested")
        FestivalRuntimeManager.stopActiveSynthesis()
    }

    override fun onSynthesizeText(request: SynthesisRequest, callback: SynthesisCallback) {
        stopRequested = false
        val originalText = request.charSequenceText?.toString() ?: request.text.orEmpty()
        val settings = WpFestivalPreferences.load(this)
        val dictionaryText = DictionaryRepository.apply(this, originalText)
        val normalizedText = SpeechTextNormalizer.normalize(
            this,
            dictionaryText,
            settings.speakEmoji,
            settings.punctuationVerbosity,
        )
        val sanitizedText = sanitizeForFestival(normalizedText)
            .replace(Regex("\\s+"), " ")
            .trim()
        val text = SpeechTextNormalizer.ensureSpeakableText(originalText, sanitizedText)
        if (text.isEmpty()) {
            callback.done()
            return
        }
        val segments = FestivalTextChunker.chunk(text)

        val voice = WpFestivalEngine.voice(request.voiceName)
            ?: WpFestivalEngine.voice(settings.voiceName)
            ?: WpFestivalEngine.defaultVoice()
        val ratePercent = mergePercent(settings.ratePercent, request.speechRate, WpFestivalRanges.RATE_MIN, WpFestivalRanges.RATE_MAX)
        val pitchPercent = mergePercent(settings.pitchPercent, request.pitch, WpFestivalRanges.PITCH_MIN, WpFestivalRanges.PITCH_MAX)
        val volumePercent = mergeVolume(settings.volumePercent, request.params)

        Log.i(
            TAG,
            "Synth start voice=${voice.name} rate=$ratePercent pitch=$pitchPercent volume=$volumePercent textLen=${text.length} segments=${segments.size}",
        )
        if (dictionaryText != originalText) {
            Log.i(TAG, "Dictionary text: $dictionaryText")
        }
        if (normalizedText != dictionaryText.trim()) {
            Log.i(TAG, "Normalized text: $normalizedText")
        }
        if (text != normalizedText.trim()) {
            Log.i(TAG, "Sanitized text: $text")
        }

        try {
            streamSegmentsToCallback(segments, voice, ratePercent, pitchPercent, volumePercent, settings, callback)
        } catch (t: Throwable) {
            Log.e(TAG, "Synthesis failed", t)
            callback.error(if (stopRequested) TextToSpeech.STOPPED else TextToSpeech.ERROR_SYNTHESIS)
            return
        }

        if (stopRequested) {
            callback.error(TextToSpeech.STOPPED)
            return
        }
        callback.done()
    }

    private fun streamSegmentsToCallback(
        segments: List<String>,
        voice: WpFestivalVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
        settings: EngineSettings,
        callback: SynthesisCallback,
    ) {
        require(segments.isNotEmpty()) { "No text segments to synthesize" }

        var sampleRate: Int? = null
        var pcmBytes = 0
        var producedAudio = false
        for ((index, segment) in segments.withIndex()) {
            if (stopRequested) {
                throw IllegalStateException("Synthesis stopped")
            }
            Log.i(TAG, "Synth segment ${index + 1}/${segments.size} len=${segment.length}")
            val segmentResult = synthesizeSegmentWithFallback(segment, voice, ratePercent, pitchPercent, volumePercent, settings)
                ?: run {
                    Log.w(TAG, "Skipping unspeakable segment ${index + 1}/${segments.size}")
                    continue
                }
            if (sampleRate == null) {
                val startResult = callback.start(segmentResult.sampleRate, AudioFormat.ENCODING_PCM_16BIT, 1)
                if (startResult != TextToSpeech.SUCCESS) {
                    if (callback.hasFinished()) {
                        stopRequested = true
                    }
                    throw IllegalStateException("TTS callback start failed: $startResult")
                }
                sampleRate = segmentResult.sampleRate
            } else if (sampleRate != segmentResult.sampleRate) {
                throw IllegalStateException("Sample rate changed between segments")
            }
            streamAudioChunk(segmentResult.pcm, callback)
            pcmBytes += segmentResult.pcm.size
            producedAudio = true
        }
        require(producedAudio) { "No segment could be synthesized" }
        Log.i(TAG, "Synth done sampleRate=$sampleRate pcmBytes=$pcmBytes")
    }

    private fun synthesizeSegmentWithFallback(
        segment: String,
        voice: WpFestivalVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
        settings: EngineSettings,
    ): SynthResult? {
        val cacheKey = SynthResultCache.keyOrNull(segment, voice.name, ratePercent, pitchPercent, volumePercent)
        cacheKey?.let { key ->
            SynthResultCache.get(key)?.let { cached ->
                Log.i(TAG, "Synth cache hit len=${segment.length}")
                return cached
            }
        }
        return try {
            FestivalRuntimeManager.synthesize(this, segment, voice, ratePercent, pitchPercent, volumePercent)
                .also { result ->
                    cacheKey?.let { SynthResultCache.put(it, result) }
                }
        } catch (primary: Throwable) {
            val fallback = SpeechTextNormalizer.ensureSpeakableText(
                segment,
                sanitizeForFestival(SpeechTextNormalizer.makeFestivalFriendly(segment, settings.punctuationVerbosity))
                    .replace(Regex("\\s+"), " ")
                    .trim(),
            )
                .replace(Regex("\\s+"), " ")
                .trim()
            if (fallback.isBlank() || fallback == segment) {
                Log.w(TAG, "Segment fallback unavailable", primary)
                null
            } else {
                Log.w(TAG, "Retrying segment with aggressive fallback: $fallback", primary)
                runCatching {
                    FestivalRuntimeManager.synthesize(this, fallback, voice, ratePercent, pitchPercent, volumePercent)
                        .also { result ->
                            cacheKey?.let { SynthResultCache.put(it, result) }
                        }
                }.getOrElse { fallbackError ->
                    Log.w(TAG, "Fallback synthesis failed", fallbackError)
                    null
                }
            }
        }
    }

    private fun streamAudioChunk(pcm: ByteArray, callback: SynthesisCallback) {
        if (pcm.isEmpty()) {
            return
        }
        val chunkSize = callback.maxBufferSize.coerceAtLeast(2048)
        var offset = 0
        while (offset < pcm.size) {
            if (stopRequested || callback.hasFinished()) {
                throw IllegalStateException("Synthesis stopped")
            }
            val size = minOf(chunkSize, pcm.size - offset)
            val status = callback.audioAvailable(pcm, offset, size)
            if (status != TextToSpeech.SUCCESS) {
                if (callback.hasFinished()) {
                    stopRequested = true
                }
                throw IllegalStateException("TTS callback audioAvailable failed: $status")
            }
            offset += size
        }
    }

    private fun mergePercent(basePercent: Int, requestPercent: Int, minPercent: Int, maxPercent: Int): Int {
        val normalizedRequest = if (requestPercent <= 0) 100 else requestPercent
        return ((basePercent.coerceIn(minPercent, maxPercent) / 100f) * normalizedRequest)
            .roundToInt()
            .coerceIn(minPercent, maxPercent)
    }

    private fun mergeVolume(basePercent: Int, params: Bundle?): Int {
        val rawVolume = params?.getFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, 1.0f) ?: 1.0f
        return (basePercent * rawVolume).roundToInt()
            .coerceIn(WpFestivalRanges.VOLUME_MIN, WpFestivalRanges.VOLUME_MAX)
    }

    private fun sanitizeForFestival(text: String): String {
        if (text.isEmpty()) {
            return text
        }
        val out = StringBuilder(text.length)
        val encoder = WINDOWS_1250.newEncoder()
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            val dropInsideWord = isWordJoinerNoise(text, index, codePoint)
            val replacement: String? = when {
                isFestivalUnsafeCodePoint(codePoint) -> if (dropInsideWord) "" else " "
                .toString()
                codePoint == '\n'.code || codePoint == '\r'.code || codePoint == '\t'.code -> if (dropInsideWord) "" else " "
                codePoint in setOf(0x2010, 0x2011, 0x2012, 0x2013, 0x2014, 0x2015, 0x2212) -> if (dropInsideWord) "" else "-"
                codePoint == 0x2026 -> "..."
                codePoint in setOf(0x2022, 0x00B7, 0x2027, 0x2219, 0x2043, 0x25CF, 0x30FB) -> if (dropInsideWord) "" else ", "
                codePoint in setOf(0x2018, 0x2019, 0x201A, 0x201B, 0x2032, 0x02BC) -> "'"
                codePoint in setOf(0x201C, 0x201D, 0x201E, 0x201F, 0x2033) -> "\""
                !encoder.canEncode(String(Character.toChars(codePoint))) -> if (dropInsideWord) "" else " "
                else -> null
            }
            if (replacement != null) {
                out.append(replacement)
            } else {
                out.appendCodePoint(codePoint)
            }
            index += Character.charCount(codePoint)
        }
        return out.toString()
    }

    private fun isWordJoinerNoise(text: String, index: Int, codePoint: Int): Boolean {
        val prev = previousNonWhitespaceCodePoint(text, index)
        val next = nextNonWhitespaceCodePoint(text, index + Character.charCount(codePoint))
        return prev != null && next != null && Character.isLetterOrDigit(prev) && Character.isLetterOrDigit(next)
    }

    private fun previousNonWhitespaceCodePoint(text: String, fromIndex: Int): Int? {
        var cursor = fromIndex
        while (cursor > 0) {
            val codePoint = text.codePointBefore(cursor)
            cursor -= Character.charCount(codePoint)
            if (!Character.isWhitespace(codePoint)) {
                return codePoint
            }
        }
        return null
    }

    private fun nextNonWhitespaceCodePoint(text: String, fromIndex: Int): Int? {
        var cursor = fromIndex
        while (cursor < text.length) {
            val codePoint = text.codePointAt(cursor)
            cursor += Character.charCount(codePoint)
            if (!Character.isWhitespace(codePoint)) {
                return codePoint
            }
        }
        return null
    }

    private fun isFestivalUnsafeCodePoint(codePoint: Int): Boolean {
        if (codePoint in 0x1F000..0x1FAFF || codePoint in 0x1F1E6..0x1F1FF) {
            return true
        }
        if (codePoint in 0x2600..0x27BF || codePoint in 0x2300..0x23FF) {
            return true
        }
        return when (Character.getType(codePoint)) {
            Character.CONTROL.toInt() -> codePoint != '\n'.code && codePoint != '\r'.code && codePoint != '\t'.code
            Character.FORMAT.toInt(),
            Character.SURROGATE.toInt(),
            Character.PRIVATE_USE.toInt(),
            Character.UNASSIGNED.toInt() -> true
            else -> false
        }
    }
}
