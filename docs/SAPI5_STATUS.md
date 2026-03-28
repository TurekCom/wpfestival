# WP Festival SAPI5 status

## Result

- The original WP/Festival voice now works as a normal SAPI5 voice on Windows 11.
- Verified in both x64 and x86 SAPI hosts.
- Pitch control now changes real F0 targets in the WP voice instead of relying on inactive `int_lr_params`.
- The package contains one real physical voice database:
  - `wp_pl_m1_diphone`
- On top of that, SAPI now exposes 7 usable voice presets:
  - `WP Festival (SAPI5)`
  - `WP Festival - Gleboki`
  - `WP Festival - Jasny`
  - `WP Festival - Wolny`
  - `WP Festival - Szybki`
  - `WP Festival - Miekki`
  - `WP Festival - Mocny`

## What was built

- New SAPI5 engine project:
  - `sapi5_wp/engine/wp_festival_sapi5.cpp`
  - `sapi5_wp/engine/wp_festival_sapi5.def`
  - `sapi5_wp/common/wp_festival_shared.h`
  - `sapi5_wp/config/wp_festival_sapi5_config.cpp`
  - `sapi5_wp/CMakeLists.txt`
- Installer scripts:
  - `install_wp_sapi5.ps1`
  - `uninstall_wp_sapi5.ps1`
  - `register_wp_com.ps1`
  - `unregister_wp_com.ps1`
  - `test_wp_sapi5.ps1`
  - `register_installed_wp_sapi5_machinewide.ps1`
  - `unregister_installed_wp_sapi5_machinewide.ps1`
  - `installer\wp_festival_sapi5_x64.iss`

## Runtime model

- The SAPI DLL does not use the old DDE layer.
- Instead it:
  1. receives text from `ISpTTSEngine`
  2. writes a temporary Scheme script
  3. launches the rebuilt local `festival.exe`
  4. synthesizes with `voice_wp_pl_m1_diphone`
  5. reads back the WAV and streams PCM to SAPI

## User settings

- Added a separate accessible settings window:
  - `WPFestivalSapi5Config.exe`
- It uses standard Win32 controls (`STATIC`, `COMBOBOX`, `TRACKBAR`, `BUTTON`) and is visible in the Windows accessibility tree.
- The SAPI token now also exposes `EngineProperties` through `ISpTokenUI`, so compatible SAPI hosts can open the same settings window directly.
- Settings are stored per-user under:
  - `HKCU\Software\WPFestival\SAPI5\Settings`
- Available settings:
  - base variant for `WP Festival (SAPI5)`
  - additional speed
  - additional pitch
  - base volume
- The named presets (`Gleboki`, `Jasny`, etc.) remain available as separate SAPI voices.

## Install command

- Machine-wide registration:

```powershell
sudo powershell -ExecutionPolicy Bypass -File .\install_wp_sapi5.ps1 -MachineWide
```

- Final normal installer:
  - `dist\installer\WPFestivalSapi5-0.3.1-x64.exe`
- Installer creates:
  - Start Menu shortcut to settings
  - Desktop shortcut to settings

## Uninstall command

```powershell
sudo powershell -ExecutionPolicy Bypass -File .\uninstall_wp_sapi5.ps1 -MachineWide
```

## Verification

- x64 SAPI enumeration shows all 7 `WP Festival*` presets.
- x86 SAPI enumeration shows all 7 `WP Festival*` presets.
- `test_wp_sapi5.ps1` produced valid WAV output.
- `WP Festival (SAPI5)` with settings variant `WPFestival.Gleboki` produces the same WAV as `WP Festival - Gleboki`.
- Pitch settings now change synthesized output and were verified on generated WAV samples.
- `ISpObjectToken::DisplayUI(..., SPDUI_EngineProperties, ...)` opens the accessible settings window.
- Generated test files:
  - `wp_sapi_test.wav`
  - `wp_sapi_test_x86.wav`
  - `installer_test_gleboki.wav`
  - `installer_test_mocny.wav`
  - `test_standard_variant_gleboki.wav`
  - `test_named_gleboki.wav`
  - `test_pitch_low.wav`
  - `test_pitch_high.wav`
  - `test_pitch_low_vowel.wav`
  - `test_pitch_high_vowel.wav`

## Notes

- `.packed` files are no longer a blocker. The working runtime uses the unpacked Scheme tree in `wp_runtime_lib`.
- The rebuilt Festival backend is still the core synthesizer; the new part is the Windows SAPI wrapper around it.
- The 7 SAPI entries are presets built on a single underlying WP voice database, not 7 separate original WP recordings.

## Female package check

- The provided file `rozmowy121 - Wersja pełna, mówiąca głosem żeńskim.exe` is byte-identical to `Rozmowy121.exe`.
- Verified by SHA256:
  - `Rozmowy121.exe` -> `9F9563557217278684EE1C81554232A59EB637CE10A89787D83F0B6B9A99D708`
  - `rozmowy121 - Wersja pełna, mówiąca głosem żeńskim.exe` -> `9F9563557217278684EE1C81554232A59EB637CE10A89787D83F0B6B9A99D708`
- That means there is no second authentic female WP database in the provided artifact.
- A separate female SAPI profile was therefore built on top of the same recovered runtime, with:
  - separate CLSIDs
  - separate token namespace `WPFestivalFemale.*`
  - separate settings root `HKCU\Software\WPFestivalFemale\SAPI5\Settings`
  - female-oriented default pitch presets
  - separate accessible config app `WPFestivalSapi5FemaleConfig.exe`
- Female installer:
  - `dist\installer\WPFestivalSapi5Female-0.4.0-x64.exe`
- Female verification:
  - x64 and x86 SAPI both enumerate `WP Festival Zenski*`
  - generated WAV files:
    - `wp_sapi_female_test.wav`
    - `wp_sapi_female_test_x86.wav`
    - `post_installer_female_test.wav`

## BlackBox-style config refresh

- Reworked both config apps to follow the vertical settings layout used in the Android `blackbox` project:
  - large header
  - variant section
  - speed / pitch / volume sliders
  - preview text area
  - `Mów`, `Stop`, `Zapisz`, `Domyślne`, `Zamknij`
- This remains a Windows desktop config app, not an Android WP port. The Android project was used as the UI model.
- Both refreshed apps still use only standard Win32 controls, so they remain visible to screen readers through the normal accessibility tree.
- Preview behavior:
  - clicking `Mów` applies current unsaved values temporarily
  - preview launches the installed WP SAPI voice, which in turn starts `festival.exe`
  - closing without `Zapisz` restores the original registry settings
- Verified with automation for both profiles:
  - male: variant changed temporarily from `WPFestival.Jasny` to `WPFestival.Standard`, `festival.exe` launched, closing restored `WPFestival.Jasny`
  - female: variant changed temporarily from `WPFestivalFemale.Miekki` to `WPFestivalFemale.Standard`, `festival.exe` launched, closing restored `WPFestivalFemale.Miekki`
- Updated installed config executables in:
  - `C:\Program Files\WP Festival SAPI5\WPFestivalSapi5Config.exe`
  - `C:\Program Files\WP Festival SAPI5 Female\WPFestivalSapi5FemaleConfig.exe`
- Refreshed installers:
  - `dist\installer\WPFestivalSapi5-0.3.2-x64.exe`
  - `dist\installer\WPFestivalSapi5Female-0.4.1-x64.exe`
