# Architecture overview

The public repository is split into two wrapper targets built around the same legacy voice runtime assumptions.

## Common model

- The original WP voice payload is treated as a local-only runtime dependency.
- Festival / Speech Tools sources are kept in `third_party/` so the open components can be rebuilt locally.
- Repository code focuses on wrappers, packaging, text normalization, configuration UI, and platform integration.

## Windows

Located under `windows/`.

- `windows/sapi5/`
  COM-based SAPI5 engine DLLs and the accessible configuration UI.
- `windows/scripts/`
  registration, install, uninstall, and smoke-test helpers.
- `windows/installer/`
  Inno Setup packaging scripts.

The Windows wrapper expects a local `runtime/festival.exe` and `runtime/wp_runtime_lib/` tree. Installer scripts package those local inputs beside the rebuilt SAPI binaries.

## Android

Located under `android/`.

- Kotlin app and `TextToSpeechService`
- settings activity inspired by the BlackBox project
- text normalization pipeline for emoji, punctuation, diacritics, and unsupported scripts
- native backend loaded from `jniLibs/`

The Android target expects locally rebuilt native libraries in `android/app/src/main/jniLibs/` and a local `android/app/src/main/assets/runtime/common/wp_runtime_lib/`.

## Why the repo is source-only

Redistribution status of the recovered WP assets is unclear. Because of that, the public repository and public GitHub releases include only:

- wrapper code
- build scripts
- installer scripts
- documentation
- upstream open-source sources

They do not include proprietary voice data, recovered payloads, signed installers, or signed Android packages.
