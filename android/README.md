# WP Festival Android

Android source tree for the WP Festival TTS port.

## Contains

- `TextToSpeechService`
- settings UI with:
  - male / female profile
  - variant selection
  - speed / pitch / volume
  - emoji reading
  - punctuation verbosity
  - TXT dictionary import
- TalkBack-oriented text normalization

## Not included in the public repo

- proprietary `wp_runtime_lib`
- prebuilt `jniLibs`
- release keystore

Placeholder locations are kept in the source tree:

- [jniLibs/README.md](app/src/main/jniLibs/README.md)
- [runtime/common/README.md](app/src/main/assets/runtime/common/README.md)

See [docs/BUILDING.md](../docs/BUILDING.md) for the local build process.
