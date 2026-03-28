# Changelog

This repository is published as a source-only archive of the reverse-engineering and porting work around the legacy WP/Festival voice.

## Windows SAPI5 source release 0.4.1

- accessible Win32 configuration UI
- male and female token namespaces
- preset-based voice variants
- PowerShell install and registration helpers
- Inno Setup packaging scripts adapted to the public repo layout

## Android source release 0.2.12

- Android `TextToSpeechService`
- BlackBox-inspired settings UI
- emoji and punctuation verbosity controls
- TXT dictionary import
- TalkBack-focused text normalization and fallback behavior for unsupported characters

## Repository publication

- public GitHub repository with source code for both targets
- upstream Festival / Speech Tools sources kept under `third_party/`
- local-only proprietary runtime inputs intentionally excluded
