param(
    [string]$InstallDir = $PSScriptRoot
)

$ErrorActionPreference = 'Stop'

function Set-ComRegistration {
    param(
        [Parameter(Mandatory = $true)][string]$BaseKey,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [Parameter(Mandatory = $true)][string]$Clsid,
        [Parameter(Mandatory = $true)][string]$DisplayName
    )
    $clsidKey = Join-Path $BaseKey $Clsid
    $inprocKey = Join-Path $clsidKey 'InprocServer32'
    New-Item -Path $inprocKey -Force | Out-Null
    Set-Item -Path $clsidKey -Value $DisplayName
    Set-Item -Path $inprocKey -Value $DllPath
    New-ItemProperty -Path $inprocKey -Name 'ThreadingModel' -Value 'Both' -PropertyType String -Force | Out-Null
}

function Set-VoiceToken {
    param(
        [Parameter(Mandatory = $true)][string]$BaseKey,
        [Parameter(Mandatory = $true)][hashtable]$Preset
    )
    $clsid = '{22B4CE69-FA37-4A16-93DE-04168FD73A87}'
    $uiClsid = '{51EC7519-9E37-4712-B83F-28E99342C49C}'
    $tokenKey = Join-Path $BaseKey $Preset.Token
    $attrKey = Join-Path $tokenKey 'Attributes'
    $uiKey = Join-Path $tokenKey 'UI'
    $enginePropertiesKey = Join-Path $uiKey 'EngineProperties'
    New-Item -Path $attrKey -Force | Out-Null
    New-Item -Path $enginePropertiesKey -Force | Out-Null
    Set-Item -Path $tokenKey -Value $Preset.Name
    New-ItemProperty -Path $tokenKey -Name 'CLSID' -Value $clsid -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name '409' -Value 'WP Festival Female' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseRateAdjust' -Value $Preset.Rate -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseVolume' -Value $Preset.Volume -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'PitchMean' -Value $Preset.PitchMean -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'PitchStd' -Value $Preset.PitchStd -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Name' -Value 'WP Festival Female' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Vendor' -Value 'WP reverse-engineered runtime' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Language' -Value '0415' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Gender' -Value 'Female' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Age' -Value 'Adult' -PropertyType String -Force | Out-Null
    Set-Item -Path $uiKey -Value ''
    Set-Item -Path $enginePropertiesKey -Value ''
    New-ItemProperty -Path $enginePropertiesKey -Name 'CLSID' -Value $uiClsid -PropertyType String -Force | Out-Null
}

$x64Dll = Join-Path $InstallDir 'x64\WPFestivalSapi5Female.dll'
$x86Dll = Join-Path $InstallDir 'x86\WPFestivalSapi5Female.dll'
if (!(Test-Path $x64Dll)) { throw "Brak DLL x64: $x64Dll" }
if (!(Test-Path $x86Dll)) { throw "Brak DLL x86: $x86Dll" }

$presets = @(
    @{ Token='WPFestivalFemale.Standard'; Name='WP Festival Zenski (SAPI5)'; Rate='0'; Volume='100'; PitchMean='145'; PitchStd='16' },
    @{ Token='WPFestivalFemale.Gleboki'; Name='WP Festival Zenski - Gleboki'; Rate='-1'; Volume='106'; PitchMean='136'; PitchStd='14' },
    @{ Token='WPFestivalFemale.Jasny'; Name='WP Festival Zenski - Jasny'; Rate='0'; Volume='100'; PitchMean='156'; PitchStd='18' },
    @{ Token='WPFestivalFemale.Wolny'; Name='WP Festival Zenski - Wolny'; Rate='-4'; Volume='100'; PitchMean='145'; PitchStd='16' },
    @{ Token='WPFestivalFemale.Szybki'; Name='WP Festival Zenski - Szybki'; Rate='4'; Volume='100'; PitchMean='145'; PitchStd='16' },
    @{ Token='WPFestivalFemale.Miekki'; Name='WP Festival Zenski - Miekki'; Rate='-1'; Volume='94'; PitchMean='141'; PitchStd='14' },
    @{ Token='WPFestivalFemale.Mocny'; Name='WP Festival Zenski - Mocny'; Rate='1'; Volume='116'; PitchMean='148'; PitchStd='19' }
)

Get-ChildItem 'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force

$engineClsid = '{22B4CE69-FA37-4A16-93DE-04168FD73A87}'
$uiClsid = '{51EC7519-9E37-4712-B83F-28E99342C49C}'

Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath $x64Dll -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Female Engine'
Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath $x86Dll -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Female Engine'
Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath $x64Dll -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 Female UI'
Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath $x86Dll -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 Female UI'

foreach ($preset in $presets) {
    Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -Preset $preset
    Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -Preset $preset
}
