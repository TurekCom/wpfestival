# Windows SAPI5

Contents:

- `sapi5/`
  C++ SAPI5 engine and accessible Win32 configuration UI.
- `scripts/`
  Install, uninstall, registration, and smoke-test PowerShell helpers.
- `installer/`
  Inno Setup scripts for packaging.

Expected local build outputs:

- `windows/build/x64/Release/WPFestivalSapi5.dll`
- `windows/build/x64/Release/WPFestivalSapi5Config.exe`
- `windows/build/x64/Release/WPFestivalSapi5Female.dll`
- `windows/build/x64/Release/WPFestivalSapi5FemaleConfig.exe`
- `windows/build/x86/Release/WPFestivalSapi5.dll`
- `windows/build/x86/Release/WPFestivalSapi5Female.dll`

Required local-only runtime inputs:

- `runtime/festival.exe`
- `runtime/wp_runtime_lib/`

The install scripts now default to repo-relative layout and can be used after you supply the missing runtime files.
