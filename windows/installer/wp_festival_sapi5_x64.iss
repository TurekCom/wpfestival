#define AppName "WP Festival SAPI5"
#define AppVersion "0.4.1"
#define AppPublisher "wpfestival contributors"
#define EngineDllName "WPFestivalSapi5.dll"
#define VoiceClsid "{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}"
#define UiClsid "{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}"

[Setup]
AppId={{1A73D8E4-01AB-44D4-9F9F-52FD1B37C201}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\WP Festival SAPI5
DefaultGroupName={#AppName}
OutputDir=..\..\dist\installer
OutputBaseFilename=WPFestivalSapi5-{#AppVersion}-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\x64\festival.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"

[Files]
Source: "..\build\x64\Release\{#EngineDllName}"; DestDir: "{app}\x64"; Flags: ignoreversion
Source: "..\build\x86\Release\{#EngineDllName}"; DestDir: "{app}\x86"; Flags: ignoreversion
Source: "..\build\x64\Release\WPFestivalSapi5Config.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\runtime\festival.exe"; DestDir: "{app}\x64"; Flags: ignoreversion
Source: "..\..\runtime\festival.exe"; DestDir: "{app}\x86"; Flags: ignoreversion
Source: "..\..\runtime\wp_runtime_lib\*"; DestDir: "{app}\x64\wp_runtime_lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\runtime\wp_runtime_lib\*"; DestDir: "{app}\x86\wp_runtime_lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\docs\SAPI5_STATUS.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\scripts\register_installed_wp_sapi5_machinewide.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\scripts\unregister_installed_wp_sapi5_machinewide.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Ustawienia WP Festival SAPI5"; Filename: "{app}\WPFestivalSapi5Config.exe"
Name: "{commondesktop}\WP Festival SAPI5 - Ustawienia"; Filename: "{app}\WPFestivalSapi5Config.exe"
Name: "{group}\Odinstaluj WP Festival SAPI5"; Filename: "{uninstallexe}"

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\register_installed_wp_sapi5_machinewide.ps1"" -InstallDir ""{app}"""; Flags: runhidden waituntilterminated
Filename: "{app}\WPFestivalSapi5Config.exe"; Description: "Otwórz ustawienia WP Festival SAPI5"; Flags: postinstall skipifsilent unchecked

[UninstallRun]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\unregister_installed_wp_sapi5_machinewide.ps1"" -InstallDir ""{app}"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregisterWPFestivalSapi5"

[Code]
type
  TVoicePreset = record
    Token: String;
    Name: String;
    Rate: String;
    Volume: String;
    PitchMean: String;
    PitchStd: String;
  end;

var
  Presets: array of TVoicePreset;

procedure AddPreset(Index: Integer; Token, Name, Rate, Volume, PitchMean, PitchStd: String);
begin
  Presets[Index].Token := Token;
  Presets[Index].Name := Name;
  Presets[Index].Rate := Rate;
  Presets[Index].Volume := Volume;
  Presets[Index].PitchMean := PitchMean;
  Presets[Index].PitchStd := PitchStd;
end;

procedure InitPresets;
begin
  SetArrayLength(Presets, 7);
  AddPreset(0, 'WPFestival.Standard', 'WP Festival (SAPI5)', '0', '100', '105', '14');
  AddPreset(1, 'WPFestival.Gleboki', 'WP Festival - Gleboki', '-1', '108', '92', '12');
  AddPreset(2, 'WPFestival.Jasny', 'WP Festival - Jasny', '0', '100', '122', '16');
  AddPreset(3, 'WPFestival.Wolny', 'WP Festival - Wolny', '-4', '100', '105', '14');
  AddPreset(4, 'WPFestival.Szybki', 'WP Festival - Szybki', '4', '100', '105', '14');
  AddPreset(5, 'WPFestival.Miekki', 'WP Festival - Miekki', '-1', '92', '98', '10');
  AddPreset(6, 'WPFestival.Mocny', 'WP Festival - Mocny', '1', '118', '110', '18');
end;

procedure RegisterComClass(BaseKey, DllPath: String);
var
  ClsidKey: String;
begin
  ClsidKey := BaseKey + '\' + '{#VoiceClsid}';
  RegWriteStringValue(HKLM, ClsidKey, '', 'WP Festival SAPI5 Engine');
  RegWriteStringValue(HKLM, ClsidKey + '\InprocServer32', '', DllPath);
  RegWriteStringValue(HKLM, ClsidKey + '\InprocServer32', 'ThreadingModel', 'Both');
end;

procedure RegisterUiClass(BaseKey, DllPath: String);
var
  ClsidKey: String;
begin
  ClsidKey := BaseKey + '\' + '{#UiClsid}';
  RegWriteStringValue(HKLM, ClsidKey, '', 'WP Festival SAPI5 UI');
  RegWriteStringValue(HKLM, ClsidKey + '\InprocServer32', '', DllPath);
  RegWriteStringValue(HKLM, ClsidKey + '\InprocServer32', 'ThreadingModel', 'Both');
end;

procedure RegisterVoiceToken(BaseKey: String; Preset: TVoicePreset);
var
  TokenKey: String;
  AttrKey: String;
  UiKey: String;
  EnginePropertiesKey: String;
begin
  TokenKey := BaseKey + '\' + Preset.Token;
  AttrKey := TokenKey + '\Attributes';
  UiKey := TokenKey + '\UI';
  EnginePropertiesKey := UiKey + '\EngineProperties';
  RegWriteStringValue(HKLM, TokenKey, '', Preset.Name);
  RegWriteStringValue(HKLM, TokenKey, 'CLSID', '{#VoiceClsid}');
  RegWriteStringValue(HKLM, TokenKey, '409', 'WP Festival');
  RegWriteStringValue(HKLM, TokenKey, 'BaseRateAdjust', Preset.Rate);
  RegWriteStringValue(HKLM, TokenKey, 'BaseVolume', Preset.Volume);
  RegWriteStringValue(HKLM, TokenKey, 'PitchMean', Preset.PitchMean);
  RegWriteStringValue(HKLM, TokenKey, 'PitchStd', Preset.PitchStd);
  RegWriteStringValue(HKLM, AttrKey, 'Name', 'WP Festival');
  RegWriteStringValue(HKLM, AttrKey, 'Vendor', 'WP reverse-engineered runtime');
  RegWriteStringValue(HKLM, AttrKey, 'Language', '0415');
  RegWriteStringValue(HKLM, AttrKey, 'Gender', 'Male');
  RegWriteStringValue(HKLM, AttrKey, 'Age', 'Adult');
  RegWriteStringValue(HKLM, UiKey, '', '');
  RegWriteStringValue(HKLM, EnginePropertiesKey, '', '');
  RegWriteStringValue(HKLM, EnginePropertiesKey, 'CLSID', '{#UiClsid}');
end;

procedure RemoveVoiceTokens(Hive: Integer; BaseKey: String);
var
  I: Integer;
begin
  for I := 0 to GetArrayLength(Presets) - 1 do
  begin
    RegDeleteKeyIncludingSubkeys(Hive, BaseKey + '\' + Presets[I].Token);
  end;
end;

procedure RemovePerUserDuplicates;
begin
  RemoveVoiceTokens(HKCU, 'SOFTWARE\Microsoft\Speech\Voices\Tokens');
  RemoveVoiceTokens(HKCU, 'SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens');
end;

procedure RegisterAllVoices;
var
  I: Integer;
begin
  RegisterComClass('SOFTWARE\Classes\CLSID', ExpandConstant('{app}\x64\{#EngineDllName}'));
  RegisterComClass('SOFTWARE\Classes\WOW6432Node\CLSID', ExpandConstant('{app}\x86\{#EngineDllName}'));
  RegisterUiClass('SOFTWARE\Classes\CLSID', ExpandConstant('{app}\x64\{#EngineDllName}'));
  RegisterUiClass('SOFTWARE\Classes\WOW6432Node\CLSID', ExpandConstant('{app}\x86\{#EngineDllName}'));

  for I := 0 to GetArrayLength(Presets) - 1 do
  begin
    RegisterVoiceToken('SOFTWARE\Microsoft\Speech\Voices\Tokens', Presets[I]);
    RegisterVoiceToken('SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens', Presets[I]);
  end;
end;

procedure UnregisterAllVoices;
begin
  RemoveVoiceTokens(HKLM, 'SOFTWARE\Microsoft\Speech\Voices\Tokens');
  RemoveVoiceTokens(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Classes\CLSID\{#VoiceClsid}');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Classes\WOW6432Node\CLSID\{#VoiceClsid}');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Classes\CLSID\{#UiClsid}');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Classes\WOW6432Node\CLSID\{#UiClsid}');
  RemovePerUserDuplicates;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    InitPresets;
    RemovePerUserDuplicates;
    RegisterAllVoices;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    InitPresets;
    UnregisterAllVoices;
  end;
end;
