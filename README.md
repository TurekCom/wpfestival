# wpfestival

Reverse-engineered tooling and wrappers around the legacy Polish WP/Festival voice, prepared in two forms:

- Windows SAPI5 wrapper with accessible configuration UI
- Android `TextToSpeechService` with TalkBack-oriented text normalization

This repository is intentionally **source-only**. It does **not** contain the proprietary WP runtime, voice database, recovered installers, signed binaries, or release keys.

## Repository layout

- `windows/`
  Windows SAPI5 engine, config UI, installer scripts, PowerShell install helpers.
- `android/`
  Android app source for the TTS engine and settings UI.
- `third_party/`
  Upstream Festival and Edinburgh Speech Tools sources used as the base for local rebuilds.
- `docs/`
  Build notes, legal notes, and historical project status snapshots.
- `runtime/`
  Placeholder location for locally supplied proprietary runtime files. Not tracked in releases.

## What is not included

- original WP installer payloads
- recovered `wp_runtime_lib`
- rebuilt `festival.exe`
- Android `jniLibs` outputs
- signed APK/AAB files
- signed Windows installers

Those parts are excluded on purpose. See [docs/LEGAL.md](docs/LEGAL.md) and [docs/BUILDING.md](docs/BUILDING.md).

## Components

### Windows SAPI5

Located in [windows/README.md](windows/README.md).

Highlights:
- accessible Win32 configuration UI
- separate male and female token namespaces
- preset-based variants
- machine-wide or per-user registration scripts

### Android TTS

Located in [android/README.md](android/README.md).

Highlights:
- `TextToSpeechService` for Android
- settings UI inspired by BlackBox
- emoji and punctuation verbosity controls
- aggressive text normalization for old Festival runtime

## Historical notes

The original working notes from the reverse-engineering session are preserved as:

- [ANALYSIS.md](docs/ANALYSIS.md)
- [SAPI5_STATUS.md](docs/SAPI5_STATUS.md)
- [ANDROID_WP_FESTIVAL_STATUS.md](docs/ANDROID_WP_FESTIVAL_STATUS.md)

Additional repository documentation:

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [CHANGELOG.md](CHANGELOG.md)

## License

Repository glue code and documentation are provided under the MIT license in [LICENSE](LICENSE).

Upstream Festival and Speech Tools keep their original licenses in `third_party/`.
