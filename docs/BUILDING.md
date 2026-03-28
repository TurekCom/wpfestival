# Building

This repository is not a turnkey binary distribution. To produce working Windows or Android builds you must provide the missing proprietary WP runtime locally.

## Required local-only inputs

- recovered `wp_runtime_lib`
- rebuilt `festival.exe` for Windows SAPI packaging
- rebuilt Android `libwpfestival_exec.so` for each ABI you want to ship
- your own signing keys if you want signed installers or Android releases

Recommended local layout:

- `runtime/wp_runtime_lib`
- `runtime/festival.exe`
- `windows/build/x64/Release/...`
- `windows/build/x86/Release/...`
- `android/app/src/main/jniLibs/arm64-v8a/libwpfestival_exec.so`
- `android/app/src/main/jniLibs/x86_64/libwpfestival_exec.so`

## Windows SAPI5

1. Build Festival / Speech Tools locally from `third_party/` or your own upstream tree.
2. Build the SAPI projects from `windows/sapi5/` with CMake and MSVC.
3. Place the resulting binaries under:
   - `windows/build/x64/Release/`
   - `windows/build/x86/Release/`
4. Place proprietary runtime files under:
   - `runtime/wp_runtime_lib`
   - `runtime/festival.exe`
5. Run one of:
   - `windows/scripts/install_wp_sapi5.ps1`
   - `windows/scripts/install_wp_sapi5_female.ps1`

## Android

1. Install Android SDK platform 36, build-tools 36.x, and NDK 28.2.
2. Rebuild native Festival backend from the upstream sources in `third_party/`.
3. Copy locally built `.so` outputs into `android/app/src/main/jniLibs/`.
4. Copy proprietary runtime assets into:
   - `android/app/src/main/assets/runtime/common/wp_runtime_lib`
5. From `android/`, run:
   - `gradlew.bat assembleDebug`
   - or `gradlew.bat assembleRelease`

## Notes

- The Android app is source-complete for the Java/Kotlin layer, but not self-contained without local runtime assets.
- The Windows installer scripts are prepared for repo-relative layout and intentionally fail fast when the proprietary inputs are missing.
