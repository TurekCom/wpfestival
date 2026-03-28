package com.turek.wpfestival.android

import android.speech.tts.TextToSpeech
import android.speech.tts.Voice
import java.util.Locale

enum class VoiceFamily(val id: String, val displayName: String) {
    MALE("male", "Męski"),
    FEMALE("female", "Żeński"),
}

data class WpFestivalPreset(
    val id: String,
    val displayName: String,
    val baseRateAdjust: Int,
    val baseVolumePercent: Int,
    val pitchMean: Int,
    val pitchStd: Int,
)

data class WpFestivalVoiceSpec(
    val name: String,
    val family: VoiceFamily,
    val preset: WpFestivalPreset,
) {
    val displayName: String = "WP Festival ${family.displayName} - ${preset.displayName}"
}

object WpFestivalEngine {
    val LOCALE: Locale = Locale.Builder().setLanguage("pl").setRegion("PL").build()
    private const val CHECK_VOICE_DATA_ENTRY = "pol-POL"

    private val languageCodes = setOf("pl", "pol")
    private val countryCodes = setOf("", "pl", "pol")

    private val malePresets = listOf(
        WpFestivalPreset("standard", "Standard", 0, 100, 105, 14),
        WpFestivalPreset("gleboki", "Głęboki", -1, 108, 92, 12),
        WpFestivalPreset("jasny", "Jasny", 0, 100, 122, 16),
        WpFestivalPreset("wolny", "Wolny", -4, 100, 105, 14),
        WpFestivalPreset("szybki", "Szybki", 4, 100, 105, 14),
        WpFestivalPreset("miekki", "Miękki", -1, 92, 98, 10),
        WpFestivalPreset("mocny", "Mocny", 1, 118, 110, 18),
    )

    private val femalePresets = listOf(
        WpFestivalPreset("standard", "Standard", 0, 100, 145, 16),
        WpFestivalPreset("gleboki", "Głęboki", -1, 106, 136, 14),
        WpFestivalPreset("jasny", "Jasny", 0, 100, 156, 18),
        WpFestivalPreset("wolny", "Wolny", -4, 100, 145, 16),
        WpFestivalPreset("szybki", "Szybki", 4, 100, 145, 16),
        WpFestivalPreset("miekki", "Miękki", -1, 94, 141, 14),
        WpFestivalPreset("mocny", "Mocny", 1, 116, 148, 19),
    )

    val voices: List<WpFestivalVoiceSpec> = buildList {
        malePresets.forEach { preset ->
            val name = "wp_festival_pl_${VoiceFamily.MALE.id}_${preset.id}"
            add(
                WpFestivalVoiceSpec(
                    name = name,
                    family = VoiceFamily.MALE,
                    preset = preset,
                ),
            )
        }
        femalePresets.forEach { preset ->
            val name = "wp_festival_pl_${VoiceFamily.FEMALE.id}_${preset.id}"
            add(
                WpFestivalVoiceSpec(
                    name = name,
                    family = VoiceFamily.FEMALE,
                    preset = preset,
                ),
            )
        }
    }

    fun defaultVoice(): WpFestivalVoiceSpec = voices.first()

    fun checkVoiceDataEntries(): List<String> = listOf(CHECK_VOICE_DATA_ENTRY)

    fun voice(name: String?): WpFestivalVoiceSpec? = voices.firstOrNull { it.name == name }

    fun presetsForFamily(family: VoiceFamily): List<WpFestivalPreset> =
        when (family) {
            VoiceFamily.MALE -> malePresets
            VoiceFamily.FEMALE -> femalePresets
        }

    fun voiceFor(family: VoiceFamily, presetId: String): WpFestivalVoiceSpec =
        voices.firstOrNull { it.family == family && it.preset.id == presetId } ?: defaultVoice()

    fun toAndroidVoice(spec: WpFestivalVoiceSpec): Voice =
        Voice(
            spec.name,
            LOCALE,
            Voice.QUALITY_NORMAL,
            Voice.LATENCY_NORMAL,
            false,
            setOf(TextToSpeech.Engine.KEY_FEATURE_EMBEDDED_SYNTHESIS),
        )

    fun iso3Language(): String = try {
        LOCALE.isO3Language
    } catch (_: Exception) {
        "pol"
    }

    fun iso3Country(): String = try {
        LOCALE.isO3Country
    } catch (_: Exception) {
        "POL"
    }

    fun matches(language: String?, country: String?): Boolean {
        val lang = (language ?: "").lowercase(Locale.ROOT)
        val ctr = (country ?: "").lowercase(Locale.ROOT)
        return lang in languageCodes && ctr in countryCodes
    }
}
