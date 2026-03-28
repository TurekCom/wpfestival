package com.turek.wpfestival.android;

import org.junit.Test;

import java.util.List;

import static org.junit.Assert.assertTrue;

public class FestivalTextChunkerTest {
    @Test
    public void longNotificationIsSplitIntoShorterChunks() {
        String normalized = SpeechTextNormalizer.INSTANCE.normalize(
            "Facebook | • | 4 min | Kamil Andrzej Mross - Psychoterapia POP & IFS | utworzył(a) wydarzenie, w którym możesz chcieć wziąć udział. | opcje odkładania powiadomień",
            true
        );
        List<String> chunks = FestivalTextChunker.INSTANCE.chunk(normalized, 120);
        assertTrue(chunks.size() >= 2);
        for (String chunk : chunks) {
            assertTrue(chunk.length() <= 120);
        }
    }
}
