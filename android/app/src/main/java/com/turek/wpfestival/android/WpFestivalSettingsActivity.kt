package com.turek.wpfestival.android

import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.util.Log
import android.view.View
import android.view.accessibility.AccessibilityEvent
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.SeekBar
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.view.ViewCompat
import com.turek.wpfestival.android.databinding.ActivitySettingsBinding

class WpFestivalSettingsActivity : ComponentActivity(), TextToSpeech.OnInitListener {
    companion object {
        private const val TAG = "WpFestivalSettings"
    }

    private lateinit var binding: ActivitySettingsBinding
    private var settings = EngineSettings()
    private var previewTts: TextToSpeech? = null
    private var previewReady = false
    private val families = listOf(VoiceFamily.MALE, VoiceFamily.FEMALE)
    private val punctuationModes = listOf(
        PunctuationVerbosity.ALL,
        PunctuationVerbosity.SOME,
        PunctuationVerbosity.MOST,
        PunctuationVerbosity.NONE,
    )
    private var currentPresets = emptyList<WpFestivalPreset>()
    private val pickDictionary = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri == null) {
            return@registerForActivityResult
        }
        runCatching {
            val name = DictionaryRepository.importFromUri(this, uri)
            settings = settings.copy(dictionaryName = name)
            WpFestivalPreferences.save(this, settings)
            updateDictionaryStatus(name)
            announce(getString(R.string.dictionary_loaded, name))
        }.onFailure {
            updateDictionaryStatus(settings.dictionaryName)
            announce(getString(R.string.dictionary_error))
            Log.e(TAG, "Dictionary import failed", it)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivitySettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)
        settings = WpFestivalPreferences.load(this)
        previewTts = TextToSpeech(this, this, packageName)

        setupVoiceSelectors()
        bindSeekBar(
            seekBar = binding.rateSeek,
            valueView = binding.rateValue,
            initialValue = settings.ratePercent,
            minValue = WpFestivalRanges.RATE_MIN,
            maxValue = WpFestivalRanges.RATE_MAX,
        ) { value ->
            settings = settings.copy(ratePercent = value)
            persistSettings()
        }
        bindSeekBar(
            seekBar = binding.pitchSeek,
            valueView = binding.pitchValue,
            initialValue = settings.pitchPercent,
            minValue = WpFestivalRanges.PITCH_MIN,
            maxValue = WpFestivalRanges.PITCH_MAX,
        ) { value ->
            settings = settings.copy(pitchPercent = value)
            persistSettings()
        }
        bindSeekBar(
            seekBar = binding.volumeSeek,
            valueView = binding.volumeValue,
            initialValue = settings.volumePercent,
            minValue = WpFestivalRanges.VOLUME_MIN,
            maxValue = WpFestivalRanges.VOLUME_MAX,
        ) { value ->
            settings = settings.copy(volumePercent = value)
            persistSettings()
        }
        setupPunctuationSpinner()
        binding.speakEmojiCheck.isChecked = settings.speakEmoji
        binding.speakEmojiCheck.setOnCheckedChangeListener { _, checked ->
            settings = settings.copy(speakEmoji = checked)
            persistSettings()
            announce(
                if (checked) getString(R.string.emoji_enabled)
                else getString(R.string.emoji_disabled),
            )
        }
        updateDictionaryStatus(settings.dictionaryName)
        binding.importDictionaryButton.setOnClickListener {
            pickDictionary.launch(arrayOf("text/plain", "text/*", "application/octet-stream"))
        }

        binding.previewSpeakButton.setOnClickListener { speakPreview() }
        binding.previewStopButton.setOnClickListener { previewTts?.stop() }
        refreshRuntimeStatus()
    }

    override fun onInit(status: Int) {
        val tts = previewTts ?: return
        if (status == TextToSpeech.SUCCESS) {
            tts.language = WpFestivalEngine.LOCALE
            previewReady = true
            applyPreviewVoice()
            Log.i(TAG, "Preview TTS initialized")
            announce(getString(R.string.preview_ready))
        } else {
            previewReady = false
            Log.e(TAG, "Preview TTS initialization failed with status=$status")
            announce(getString(R.string.preview_error))
        }
    }

    override fun onDestroy() {
        previewTts?.stop()
        previewTts?.shutdown()
        previewTts = null
        super.onDestroy()
    }

    private fun setupVoiceSelectors() {
        val selectedVoice = WpFestivalEngine.voice(settings.voiceName) ?: WpFestivalEngine.defaultVoice()
        val familyAdapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            families.map { it.displayName },
        )
        familyAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.voiceFamilySpinner.adapter = familyAdapter
        binding.voiceFamilySpinner.setSelection(families.indexOf(selectedVoice.family), false)
        updateVariantSpinner(selectedVoice.family, selectedVoice.preset.id)

        binding.voiceFamilySpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                val family = families.getOrNull(position) ?: VoiceFamily.MALE
                updateVariantSpinner(family, currentVoice().preset.id)
                persistSettings()
                announce("Wybrano typ głosu: ${family.displayName}.")
            }

            override fun onNothingSelected(parent: AdapterView<*>?) = Unit
        }

        binding.variantSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                persistSettings()
                announce("Wybrano wariant: ${currentVoice().preset.displayName}.")
            }

            override fun onNothingSelected(parent: AdapterView<*>?) = Unit
        }
    }

    private fun updateVariantSpinner(family: VoiceFamily, selectedPresetId: String) {
        currentPresets = WpFestivalEngine.presetsForFamily(family)
        val adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            currentPresets.map { it.displayName },
        )
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.variantSpinner.adapter = adapter
        val index = currentPresets.indexOfFirst { it.id == selectedPresetId }.takeIf { it >= 0 } ?: 0
        binding.variantSpinner.setSelection(index, false)
    }

    private fun setupPunctuationSpinner() {
        val adapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            punctuationModes.map { punctuationVerbosityLabel(it) },
        )
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.punctuationSpinner.adapter = adapter
        val selectedIndex = punctuationModes.indexOf(settings.punctuationVerbosity).takeIf { it >= 0 } ?: 3
        binding.punctuationSpinner.setSelection(selectedIndex, false)
        binding.punctuationSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                val punctuationVerbosity = punctuationModes.getOrNull(position) ?: PunctuationVerbosity.NONE
                if (settings.punctuationVerbosity != punctuationVerbosity) {
                    settings = settings.copy(punctuationVerbosity = punctuationVerbosity)
                    persistSettings()
                    announce(getString(R.string.punctuation_selected, punctuationVerbosityLabel(punctuationVerbosity)))
                }
            }

            override fun onNothingSelected(parent: AdapterView<*>?) = Unit
        }
    }

    private fun currentVoice(): WpFestivalVoiceSpec {
        val family = families.getOrNull(binding.voiceFamilySpinner.selectedItemPosition) ?: VoiceFamily.MALE
        val preset = currentPresets.getOrNull(binding.variantSpinner.selectedItemPosition)
            ?: WpFestivalEngine.presetsForFamily(family).first()
        return WpFestivalEngine.voiceFor(family, preset.id)
    }

    private fun persistSettings() {
        settings = settings.copy(voiceName = currentVoice().name)
        WpFestivalPreferences.save(this, settings)
        if (previewReady) {
            applyPreviewVoice()
        }
    }

    private fun applyPreviewVoice() {
        val tts = previewTts ?: return
        tts.language = WpFestivalEngine.LOCALE
        val fallbackVoice = WpFestivalEngine.voice(settings.voiceName) ?: WpFestivalEngine.defaultVoice()
        val voice = runCatching {
            tts.voices?.firstOrNull { it.name == settings.voiceName }
                ?: WpFestivalEngine.toAndroidVoice(fallbackVoice)
        }.getOrElse {
            Log.w(TAG, "Could not query preview voices, using fallback", it)
            WpFestivalEngine.toAndroidVoice(fallbackVoice)
        }
        runCatching {
            tts.voice = voice
            Log.i(TAG, "Applied preview voice=${voice.name}")
        }.onFailure {
            Log.w(TAG, "Failed to apply preview voice=${voice.name}", it)
        }
    }

    private fun refreshRuntimeStatus() {
        binding.runtimeStatus.text = getString(R.string.runtime_preparing)
        Thread {
            val text = runCatching {
                getString(R.string.runtime_ready, FestivalRuntimeManager.runtimeStatus(this))
            }.getOrElse {
                getString(R.string.runtime_error)
            }
            runOnUiThread { binding.runtimeStatus.text = text }
        }.start()
    }

    private fun bindSeekBar(
        seekBar: SeekBar,
        valueView: TextView,
        initialValue: Int,
        minValue: Int,
        maxValue: Int,
        onChanged: (Int) -> Unit,
    ) {
        seekBar.min = minValue
        seekBar.max = maxValue
        seekBar.progress = initialValue.coerceIn(minValue, maxValue)
        updatePercentText(seekBar, valueView, seekBar.progress)
        seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {
                updatePercentText(seekBar, valueView, progress)
                if (fromUser) {
                    onChanged(progress)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar) = Unit

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                announce(getString(R.string.percent_value, seekBar.progress))
            }
        })
    }

    private fun updatePercentText(seekBar: SeekBar, valueView: TextView, value: Int) {
        val text = getString(R.string.percent_value, value)
        valueView.text = text
        ViewCompat.setStateDescription(seekBar, text)
    }

    private fun updateDictionaryStatus(name: String?) {
        binding.dictionaryStatus.text = if (name.isNullOrBlank()) {
            getString(R.string.dictionary_none)
        } else {
            getString(R.string.dictionary_loaded, name)
        }
    }

    private fun speakPreview() {
        val text = binding.previewEdit.text?.toString()?.trim().orEmpty()
        if (text.isEmpty()) {
            announce(getString(R.string.preview_empty))
            return
        }
        if (!previewReady) {
            announce(getString(R.string.preview_error))
            return
        }
        persistSettings()
        previewTts?.stop()
        Log.i(TAG, "Starting preview for ${settings.voiceName}")
        previewTts?.speak(text, TextToSpeech.QUEUE_FLUSH, null, "wp-festival-preview")
    }

    private fun announce(text: String) {
        window.decorView.announceForAccessibility(text)
        window.decorView.sendAccessibilityEvent(AccessibilityEvent.TYPE_ANNOUNCEMENT)
    }

    private fun punctuationVerbosityLabel(verbosity: PunctuationVerbosity): String = when (verbosity) {
        PunctuationVerbosity.ALL -> getString(R.string.punctuation_all)
        PunctuationVerbosity.SOME -> getString(R.string.punctuation_some)
        PunctuationVerbosity.MOST -> getString(R.string.punctuation_most)
        PunctuationVerbosity.NONE -> getString(R.string.punctuation_none)
    }
}
