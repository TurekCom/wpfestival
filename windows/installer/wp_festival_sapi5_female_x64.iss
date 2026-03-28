#define AppName "WP Festival SAPI5 Female"
#define AppVersion "0.4.1"
#define AppPublisher "wpfestival contributors"
#define EngineDllName "WPFestivalSapi5Female.dll"
#define ConfigExeName "WPFestivalSapi5FemaleConfig.exe"
#define VoiceClsid "{22B4CE69-FA37-4A16-93DE-04168FD73A87}"
#define UiClsid "{51EC7519-9E37-4712-B83F-28E99342C49C}"

[Setup]
AppId={{1EEDCE7F-655D-4B78-9227-FF1FCA29E87A}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\WP Festival SAPI5 Female
DefaultGroupName={#AppName}
OutputDir=..\..\dist\installer
OutputBaseFilename=WPFestivalSapi5Female-{#AppVersion}-x64
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
Source: "..\build\x64\Release\{#ConfigExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\runtime\festival.exe"; DestDir: "{app}\x64"; Flags: ignoreversion
Source: "..\..\runtime\festival.exe"; DestDir: "{app}\x86"; Flags: ignoreversion
Source: "..\..\runtime\wp_runtime_lib\*"; DestDir: "{app}\x64\wp_runtime_lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\runtime\wp_runtime_lib\*"; DestDir: "{app}\x86\wp_runtime_lib"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\docs\SAPI5_STATUS.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\scripts\register_installed_wp_sapi5_female_machinewide.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\scripts\unregister_installed_wp_sapi5_female_machinewide.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Ustawienia WP Festival SAPI5 Female"; Filename: "{app}\{#ConfigExeName}"
Name: "{commondesktop}\WP Festival SAPI5 Female - Ustawienia"; Filename: "{app}\{#ConfigExeName}"
Name: "{group}\Odinstaluj WP Festival SAPI5 Female"; Filename: "{uninstallexe}"

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\register_installed_wp_sapi5_female_machinewide.ps1"" -InstallDir ""{app}"""; Flags: runhidden waituntilterminated
Filename: "{app}\{#ConfigExeName}"; Description: "Otwórz ustawienia WP Festival SAPI5 Female"; Flags: postinstall skipifsilent unchecked

[UninstallRun]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-ExecutionPolicy Bypass -File ""{app}\unregister_installed_wp_sapi5_female_machinewide.ps1"" -InstallDir ""{app}"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregisterWPFestivalSapi5Female"

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
  AddPreset(0, 'WPFestivalFemale.Standard', 'WP Festival Zenski (SAPI5)', '0', '100', '145', '16');
  AddPreset(1, 'WPFestivalFemale.Gleboki', 'WP Festival Zenski - Gleboki', '-1', '106', '136', '14');
  AddPreset(2, 'WPFestivalFemale.Jasny', 'WP Festival Zenski - Jasny', '0', '100', '156', '18');
  AddPreset(3, 'WPFestivalFemale.Wolny', 'WP Festival Zenski - Wolny', '-4', '100', '145', '16');
  AddPreset(4, 'WPFestivalFemale.Szybki', 'WP Festival Zenski - Szybki', '4', '100', '145', '16');
  AddPreset(5, 'WPFestivalFemale.Miekki', 'WP Festival Zenski - Miekki', '-1', '94', '141', '14');
  AddPreset(6, 'WPFestivalFemale.Mocny', 'WP Festival Zenski - Mocny', '1', '116', '148', '19');
end;

procedure RegisterComClass(BaseKey, DllPath: String);
var
  ClsidKey: String;
begin
  ClsidKey := BaseKey + '\' + '{#VoiceClsid}';
  RegWriteStringValue(HKLM, ClsidKey, '', 'WP Festival SAPI5 Female Engine');
  RegWriteStringValue(HKLM, ClsidKey + '\InprocServer32', '', DllPath);
  RegWriteStringValue(HKLM, ClsidKey + '\InprocServer32', 'ThreadingModel', 'Both');
end;

procedure RegisterUiClass(BaseKey, DllPath: String);
var
  ClsidKey: String;
begin
  ClsidKey := BaseKey + '\' + '{#UiClsid}';
  RegWriteStringValue(HKLM, ClsidKey, '', 'WP Festival SAPI5 Female UI');
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
  RegWriteStringValue(HKLM, TokenKey, '409', 'WP Festival Female');
  RegWriteStringValue(HKLM, TokenKey, 'BaseRateAdjust', Preset.Rate);
  RegWriteStringValue(HKLM, TokenKey, 'BaseVolume', Preset.Volume);
  RegWriteStringValue(HKLM, TokenKey, 'PitchMean', Preset.PitchMean);
  RegWriteStringValue(HKLM, TokenKey, 'PitchStd', Preset.PitchStd);
  RegWriteStringValue(HKLM, AttrKey, 'Name', 'WP Festival Female');
  RegWriteStringValue(HKLM, AttrKey, 'Vendor', 'WP reverse-engineered runtime');
  RegWriteStringValue(HKLM, AttrKey, 'Language', '0415');
  RegWriteStringValue(HKLM, AttrKey, 'Gender', 'Female');
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
