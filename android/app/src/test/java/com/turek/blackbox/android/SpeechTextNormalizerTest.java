package com.turek.wpfestival.android;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class SpeechTextNormalizerTest {
    @Test
    public void singleLetterGetsPolishName() {
        assertEquals("ce", SpeechTextNormalizer.INSTANCE.normalize("c", true));
        assertEquals("be", SpeechTextNormalizer.INSTANCE.normalize("b", true));
        assertEquals("si", SpeechTextNormalizer.INSTANCE.normalize("ś", true));
    }

    @Test
    public void singleSymbolGetsPolishName() {
        assertEquals("małpa", SpeechTextNormalizer.INSTANCE.normalize("@", true));
        assertEquals("kropka", SpeechTextNormalizer.INSTANCE.normalize(".", true));
        assertEquals("kropka", SpeechTextNormalizer.INSTANCE.normalize(".", true, PunctuationVerbosity.NONE));
    }

    @Test
    public void emojiCanBeReadOrLeftAsIs() {
        assertEquals("uśmiechnięta buźka", SpeechTextNormalizer.INSTANCE.normalize("😀", true));
        assertEquals("😀", SpeechTextNormalizer.INSTANCE.normalize("😀", false));
    }

    @Test
    public void cldrEmojiMapSupportsNewerEmojiAndVariants() {
        java.util.Map<String, String> map = new java.util.LinkedHashMap<>();
        map.put("🦬", "żubr");
        map.put("👋🏻", "machająca dłoń: karnacja jasna");
        assertEquals("żubr", SpeechTextNormalizer.INSTANCE.normalize("🦬", true, map));
        assertEquals("machająca dłoń: karnacja jasna", SpeechTextNormalizer.INSTANCE.normalize("👋🏻", true, map));
    }

    @Test
    public void inlineEmojiAndAsciiEmoticonsAreExpanded() {
        assertEquals("Hej uśmiechnięta buźka.", SpeechTextNormalizer.INSTANCE.normalize("Hej 😀.", true));
        assertEquals("To jest uśmiechnięta buźka", SpeechTextNormalizer.INSTANCE.normalize("To jest :)", true));
    }

    @Test
    public void accessibilityControlsAndPlaybackRateAreNormalized() {
        assertEquals("jeden przecinek pięć razy", SpeechTextNormalizer.INSTANCE.normalize("\u200E1,5×", true));
        assertEquals(
            "TalkBack, Włączono, Używaj czytnika ekranu",
            SpeechTextNormalizer.INSTANCE.normalize("TalkBack | Włączono / Używaj czytnika ekranu", true)
        );
    }

    @Test
    public void technicalTokensAndNonBreakingSpacesAreExpanded() {
        assertEquals(
            "sftp dwukropek ukośnik ukośnik grzegorz dwukropek gwiazdka gwiazdka gwiazdka małpa 192 kropka 168 kropka 0 kropka 128 ukośnik",
            SpeechTextNormalizer.INSTANCE.normalize("sftp://grzegorz:***@192.168.0.128/", true)
        );
        assertEquals(
            "Rozmiar elementów i tekstu",
            SpeechTextNormalizer.INSTANCE.normalize("Rozmiar elementów i\u00A0tekstu", true)
        );
    }

    @Test
    public void europeanDiacriticsAreSimplified() {
        assertEquals("Muller deja facade coeur", SpeechTextNormalizer.INSTANCE.normalize("Müller déjà façade cœur", true));
        assertEquals("uber alles", SpeechTextNormalizer.INSTANCE.normalize("über alles", true));
    }

    @Test
    public void unsupportedScriptsAreDroppedInsteadOfBlockingWholeText() {
        assertEquals("Facebook powiadomienie", SpeechTextNormalizer.INSTANCE.normalize("Facebook 通知 Привет مرحبا powiadomienie", true));
        assertEquals("ikona WhatsApp", SpeechTextNormalizer.INSTANCE.normalize("ikona واتساب WhatsApp", true));
    }

    @Test
    public void typographicUiStringsAreMadeFestivalSafe() {
        assertEquals("WhatsApp, 1 powiadomienie", SpeechTextNormalizer.INSTANCE.normalize("WhatsApp – 1 powiadomienie", true));
        assertEquals("Przesłałeś nagranie głosowe.", SpeechTextNormalizer.INSTANCE.normalize("Przesłałeś(aś) nagranie głosowe.", true));
    }

    @Test
    public void standaloneUnsupportedScriptFallsBackToGenericLabel() {
        assertEquals("tekst w innym języku", SpeechTextNormalizer.INSTANCE.normalize("解说", true));
    }

    @Test
    public void weirdCharactersInsideWordAreSkippedInsteadOfBreakingWholeWord() {
        assertEquals("WhatsApp", SpeechTextNormalizer.INSTANCE.normalize("What\u200FsApp", true));
        assertEquals("bohater", SpeechTextNormalizer.INSTANCE.normalize("boהhater", true));
    }

    @Test
    public void repeatedEmojiSequencesAreCompacted() {
        assertEquals("cztery razy buźka z sercami w oczach", SpeechTextNormalizer.INSTANCE.normalize("😍😍😍😍", true));
    }

    @Test
    public void verboseNotificationLabelsAreCondensed() {
        String normalized = SpeechTextNormalizer.INSTANCE.normalize(
            "Zarządzaj ustawieniami powiadomień: Maciej Gardeła zareagował na post: „Wszystkiego najlepszego 😍😍😍😍”.",
            true
        );
        assertTrue(normalized.startsWith("Ustawienia powiadomień,"));
        assertFalse(normalized.contains("Zarządzaj ustawieniami powiadomień"));
        assertFalse(normalized.contains("buźka z sercami w oczach buźka z sercami w oczach"));
        assertFalse(normalized.contains("\""));
        assertFalse(normalized.contains("„"));
        assertFalse(normalized.contains("”"));
    }

    @Test
    public void punctuationVerbosityAffectsOnlyGeneralUtterances() {
        assertEquals(
            "Ala, ma kota.",
            SpeechTextNormalizer.INSTANCE.normalize("Ala, ma kota.", true, PunctuationVerbosity.NONE)
        );
        assertEquals(
            "Ala przecinek ma kota kropka",
            SpeechTextNormalizer.INSTANCE.normalize("Ala, ma kota.", true, PunctuationVerbosity.SOME)
        );
        assertEquals(
            "URL dwukropek test ukośnik ok",
            SpeechTextNormalizer.INSTANCE.normalize("URL: test/ok", true, PunctuationVerbosity.MOST)
        );
    }
}
