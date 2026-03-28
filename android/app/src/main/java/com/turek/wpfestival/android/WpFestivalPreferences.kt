package com.turek.wpfestival.android

import android.content.Context

object WpFestivalRanges {
    const val RATE_MIN = 0
    const val RATE_MAX = 200
    const val RATE_NEUTRAL = 100
    const val PITCH_MIN = 0
    const val PITCH_MAX = 200
    const val PITCH_NEUTRAL = 100
    const val VOLUME_MIN = 0
    const val VOLUME_MAX = 200
    const val VOLUME_NEUTRAL = 100
}

enum class PunctuationVerbosity(val storageValue: String) {
    ALL("all"),
    SOME("some"),
    MOST("most"),
    NONE("none");

    companion object {
        fun fromStorage(value: String?): PunctuationVerbosity =
            entries.firstOrNull { it.storageValue == value } ?: NONE
    }
}

data class EngineSettings(
    val voiceName: String = WpFestivalEngine.defaultVoice().name,
    val ratePercent: Int = WpFestivalRanges.RATE_NEUTRAL,
    val pitchPercent: Int = WpFestivalRanges.PITCH_NEUTRAL,
    val volumePercent: Int = WpFestivalRanges.VOLUME_NEUTRAL,
    val speakEmoji: Boolean = true,
    val punctuationVerbosity: PunctuationVerbosity = PunctuationVerbosity.NONE,
    val dictionaryName: String? = null,
)

object WpFestivalPreferences {
    private const val PREFS_NAME = "wp_festival_android_settings"
    private const val SETTINGS_VERSION = 4
    private const val KEY_SETTINGS_VERSION = "settings_version"
    private const val KEY_VOICE_NAME = "voice_name"
    private const val KEY_RATE = "rate_percent"
    private const val KEY_PITCH = "pitch_percent"
    private const val KEY_VOLUME = "volume_percent"
    private const val KEY_SPEAK_EMOJI = "speak_emoji"
    private const val KEY_PUNCTUATION_VERBOSITY = "punctuation_verbosity"
    private const val KEY_DICTIONARY_NAME = "dictionary_name"

    fun load(context: Context): EngineSettings {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val savedVersion = prefs.getInt(KEY_SETTINGS_VERSION, 1)
        val storedRate = prefs.getInt(KEY_RATE, if (savedVersion >= SETTINGS_VERSION) WpFestivalRanges.RATE_NEUTRAL else 50)
        val storedPitch = prefs.getInt(KEY_PITCH, if (savedVersion >= SETTINGS_VERSION) WpFestivalRanges.PITCH_NEUTRAL else 50)
        val storedVolume = prefs.getInt(KEY_VOLUME, WpFestivalRanges.VOLUME_NEUTRAL)
        return EngineSettings(
            voiceName = prefs.getString(KEY_VOICE_NAME, WpFestivalEngine.defaultVoice().name)
                ?: WpFestivalEngine.defaultVoice().name,
            ratePercent = if (savedVersion >= SETTINGS_VERSION) {
                storedRate.coerceIn(WpFestivalRanges.RATE_MIN, WpFestivalRanges.RATE_MAX)
            } else {
                legacyMidScaleToExtendedRange(storedRate, WpFestivalRanges.RATE_MAX)
            },
            pitchPercent = if (savedVersion >= SETTINGS_VERSION) {
                storedPitch.coerceIn(WpFestivalRanges.PITCH_MIN, WpFestivalRanges.PITCH_MAX)
            } else {
                legacyMidScaleToExtendedRange(storedPitch, WpFestivalRanges.PITCH_MAX)
            },
            volumePercent = storedVolume.coerceIn(WpFestivalRanges.VOLUME_MIN, WpFestivalRanges.VOLUME_MAX),
            speakEmoji = prefs.getBoolean(KEY_SPEAK_EMOJI, true),
            punctuationVerbosity = PunctuationVerbosity.fromStorage(
                prefs.getString(KEY_PUNCTUATION_VERBOSITY, PunctuationVerbosity.NONE.storageValue),
            ),
            dictionaryName = prefs.getString(KEY_DICTIONARY_NAME, null),
        )
    }

    fun save(context: Context, settings: EngineSettings) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_SETTINGS_VERSION, SETTINGS_VERSION)
            .putString(KEY_VOICE_NAME, settings.voiceName)
            .putInt(KEY_RATE, settings.ratePercent.coerceIn(WpFestivalRanges.RATE_MIN, WpFestivalRanges.RATE_MAX))
            .putInt(KEY_PITCH, settings.pitchPercent.coerceIn(WpFestivalRanges.PITCH_MIN, WpFestivalRanges.PITCH_MAX))
            .putInt(KEY_VOLUME, settings.volumePercent.coerceIn(WpFestivalRanges.VOLUME_MIN, WpFestivalRanges.VOLUME_MAX))
            .putBoolean(KEY_SPEAK_EMOJI, settings.speakEmoji)
            .putString(KEY_PUNCTUATION_VERBOSITY, settings.punctuationVerbosity.storageValue)
            .putString(KEY_DICTIONARY_NAME, settings.dictionaryName)
            .apply()
    }

    private fun legacyMidScaleToExtendedRange(value: Int, maxValue: Int): Int =
        (value.coerceIn(0, 100) * 2).coerceIn(0, maxValue)
}
