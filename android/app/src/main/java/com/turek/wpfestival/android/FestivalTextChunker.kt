package com.turek.wpfestival.android

object FestivalTextChunker {
    private const val DEFAULT_MAX_CHARS = 120
    private const val MIN_BREAK_SEARCH = 32

    fun chunk(text: String, maxChars: Int = DEFAULT_MAX_CHARS): List<String> {
        val normalized = text.trim()
        if (normalized.isEmpty()) {
            return emptyList()
        }
        if (normalized.length <= maxChars) {
            return listOf(normalized)
        }

        val chunks = ArrayList<String>()
        var remaining = normalized
        while (remaining.length > maxChars) {
            val splitIndex = findSplitIndex(remaining, maxChars)
            val chunk = remaining.substring(0, splitIndex).trim()
            if (chunk.isNotEmpty()) {
                chunks += chunk
            }
            remaining = remaining.substring(splitIndex).trimStart()
        }
        if (remaining.isNotBlank()) {
            chunks += remaining
        }
        return chunks
    }

    private fun findSplitIndex(text: String, maxChars: Int): Int {
        val searchStart = MIN_BREAK_SEARCH.coerceAtMost(maxChars / 2).coerceAtLeast(1)
        for (index in maxChars downTo searchStart) {
            val current = text[index - 1]
            if (current == '.' || current == '!' || current == '?' || current == ';' || current == ':') {
                return index
            }
        }
        for (index in maxChars downTo searchStart) {
            val current = text[index - 1]
            if (current == ',' || current == '-' || current.isWhitespace()) {
                return index
            }
        }
        return maxChars
    }
}
