package com.turek.wpfestival.android;

import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertEquals;

public class WpFestivalEngineTest {
    @Test
    public void checkVoiceDataUsesLocaleStyleEntriesForAndroidSettings() {
        List<String> entries = WpFestivalEngine.INSTANCE.checkVoiceDataEntries();
        assertEquals(1, entries.size());
        assertEquals("pol-POL", entries.get(0));
    }
}
