# WP / Festival analysis

## What this is

- `Rozmowy121.exe` is a 32-bit PE installer from `2001-07-10`.
- The installer payload contains NSIS markers (`nsisinstall`) and installs to:
  `C:\Program Files (x86)\Wirtualna Polska\System syntezy mowy`
- Runtime binaries are:
  - `rozmowy.exe`
  - `synteza_DDE_klient.exe`
  - `RozmowyRemove.exe`

## What was confirmed on Windows 11

- The original installer is blocked when launched directly because its Authenticode certificate is revoked.
- After copying the installer and stripping its embedded signature, the installer runs and deploys the program.
- Installed runtime binaries are not signed and can be launched on Windows 11.
- Windows 11 already provides `MFC42.DLL` in `C:\Windows\SysWOW64`, so the old VC6/MFC dependency is not the main blocker.
- The installer creates `Rozmowa.lnk` in the current user's Startup folder and points it to:
  `C:\Program Files (x86)\Wirtualna Polska\System syntezy mowy\rozmowy.exe`

## Internal structure

- This is a Festival/FestVox-style voice bundle, not just a raw Windows SAPI voice.
- Installed data contains:
  - `lib\festival.scm.packed`
  - `lib\init.scm.packed`
  - `lib\voices\polish\wp_pl_m1_diphone\festvox\*.scm.packed`
  - `lib\voices\polish\wp_pl_m1_diphone\group\m1lpc.group`
- The voice name is clearly:
  - `wp_pl_m1_diphone`
- There are also two plain text knobs:
  - `lib\tts_volume.scm`
  - `lib\voices\polish\wp_pl_m1_diphone\festvox\wp_pl_talkspeed.scm`

## Registry/runtime model

- Registry key:
  `HKLM\Software\WOW6432Node\Wirtualna Polska\SiRDDEServer`
- Observed values:
  - `festival = 1`
  - `sphinx = 0`
  - `version = 1.4`
  - `libdir = C:\Program Files (x86)\Wirtualna Polska\System syntezy mowy\lib`
  - `argfile = .\lm\lm.arg`
  - `logfile = .\sphinx.log`
- This strongly suggests a hidden DDE-based server/client model with Festival active and Sphinx disabled in this install.

## Reverse-engineering notes

- `rozmowy.exe` and `synteza_DDE_klient.exe` are 32-bit MFC applications.
- `rozmowy.exe` is UPX-packed.
- `.packed` files use a custom container beginning with magic:
  `pck\wd\`
- The `.packed` header appears to store at least:
  - uncompressed size
  - packed size
- Example:
  - `festival.scm.packed` starts with `70 63 6B 5C 77 64 5C`
- The payload is not plain gzip/zip/cab/7z/rar; it is a custom scheme and was not fully decoded here.

## New protocol findings

- `synteza_DDE_klient.exe` is not the visible public server. On startup it:
  - registers its own DDE service `synteza_DDE_klient`
  - connects to `Rozmowy|festival`
  - requests `festivalOUT`
  - starts advise on `festivalOUT`
  - pokes status to `festivalIN`
- A minimal fake `Rozmowy` server is enough to drive this handshake. Local tools added:
  - `fake_rozmowy_server.cpp`
  - `fake_rozmowy_server_x86.exe`
  - `trace_wp_api.py`
  - updated `wp_dde_probe.cpp`
- Observed handshake with the fake server:
  - `XTYP_REQUEST festivalOUT`
  - `XTYP_ADVSTART festivalOUT`
  - client status pokes on `festivalIN`
  - low-level engine requests on `wpinaczIN`

### Confirmed commands and states

- High-level command `SGetVoices` is accepted by the client on `festivalOUT`.
- After repeated advise cycles, the client emits:
  - `festival|not_initialized`
  - `festival|op_in_progress`
  - `festival|free`
  - `wpinaczIN -> SInit|1|500000|0`
  - `wpinaczIN -> SEvalCommand|(voice_rab_diphone)|0`
- High-level `SSayText|test|0|voice_wp_pl_m1_diphone|0|0` currently gets only as far as:
  - `festival|not_initialized`
  - `wpinaczIN -> SInit|1|500000|0`
  This means the missing piece is the low-level backend behind `wpinaczIN`, not the outer DDE handshake.

### Implication

- `rozmowy.exe` is not just a tray app. It likely hosts or brokers the actual low-level Festival execution requested through `wpinaczIN`.
- Replacing `rozmowy.exe` with a SAPI voice requires one of:
  1. implementing the `wpinaczIN` low-level backend (`SInit`, `SEvalCommand`, `SSay`, `STextToWave`)
  2. or decoding the WP packed Scheme layer and rebuilding the voice on upstream Festival/compatible runtime

## Packed-format notes

- In `synteza_DDE_klient_unpacked.exe`, the `.data` section contains a prefix table for:
  - `wa\`
  - `wu\`
  - `wd\`
  - `wp-ascii\`
  - `wp-unicode\`
  - `wp-data\`
  - `pck`
  - `packed`
- This confirms that packed-file handling is implemented in the WP binary itself, but the actual decompressor was not fully lifted yet.
- The generic Festival library files in `lib\*.scm.packed` appear to correspond closely to upstream Festival sources such as `festival\lib\festival.scm`, so the container is a text-oriented packer rather than encrypted content.

## Constraints

- There is no source tree in this folder.
- The license text says the WP package itself may be copied/distributed only as a whole and does not grant modification rights for the WP package.
- Festival upstream is mentioned in the license, but the WP wrapping/runtime is proprietary.

## Practical conclusion

- Running on Windows 11 is partially viable:
  - installer can be made to run by removing the revoked signature from a copy
  - deployed binaries load on Win11
  - the voice data is present and structured coherently
- Recompiling the original WP synthesizer is not currently realistic because no source code is present.
- A realistic port path would be:
  1. decode or replace the WP `.packed` Scheme layer
  2. unpack `rozmowy.exe`
  3. reconstruct the DDE protocol or bypass it
  4. evaluate whether `m1lpc.group` can be reused from upstream Festival/FestVox tooling

## Local helper

- `inspect_wp.py` reproduces the key local findings:
  - install dir
  - registry values
  - executable list
  - `.packed` header metadata
- `fake_rozmowy_server_x86.exe` reproduces the outer DDE handshake and logs `festivalIN` / `wpinaczIN`.
- `trace_wp_api.py` can trace the client and log its internal DDE callback activity.
