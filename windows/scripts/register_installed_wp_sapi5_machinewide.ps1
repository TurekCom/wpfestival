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
    $clsid = '{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}'
    $uiClsid = '{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}'
    $tokenKey = Join-Path $BaseKey $Preset.Token
    $attrKey = Join-Path $tokenKey 'Attributes'
    $uiKey = Join-Path $tokenKey 'UI'
    $enginePropertiesKey = Join-Path $uiKey 'EngineProperties'
    New-Item -Path $attrKey -Force | Out-Null
    New-Item -Path $enginePropertiesKey -Force | Out-Null
    Set-Item -Path $tokenKey -Value $Preset.Name
    New-ItemProperty -Path $tokenKey -Name 'CLSID' -Value $clsid -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name '409' -Value 'WP Festival' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseRateAdjust' -Value $Preset.Rate -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseVolume' -Value $Preset.Volume -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'PitchMean' -Value $Preset.PitchMean -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'PitchStd' -Value $Preset.PitchStd -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Name' -Value 'WP Festival' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Vendor' -Value 'WP reverse-engineered runtime' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Language' -Value '0415' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Gender' -Value 'Male' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Age' -Value 'Adult' -PropertyType String -Force | Out-Null
    Set-Item -Path $uiKey -Value ''
    Set-Item -Path $enginePropertiesKey -Value ''
    New-ItemProperty -Path $enginePropertiesKey -Name 'CLSID' -Value $uiClsid -PropertyType String -Force | Out-Null
}

$x64Dll = Join-Path $InstallDir 'x64\WPFestivalSapi5.dll'
$x86Dll = Join-Path $InstallDir 'x86\WPFestivalSapi5.dll'
if (!(Test-Path $x64Dll)) { throw "Brak DLL x64: $x64Dll" }
if (!(Test-Path $x86Dll)) { throw "Brak DLL x86: $x86Dll" }

$presets = @(
    @{ Token='WPFestival.Standard'; Name='WP Festival (SAPI5)'; Rate='0'; Volume='100'; PitchMean='105'; PitchStd='14' },
    @{ Token='WPFestival.Gleboki'; Name='WP Festival - Gleboki'; Rate='-1'; Volume='108'; PitchMean='92'; PitchStd='12' },
    @{ Token='WPFestival.Jasny'; Name='WP Festival - Jasny'; Rate='0'; Volume='100'; PitchMean='122'; PitchStd='16' },
    @{ Token='WPFestival.Wolny'; Name='WP Festival - Wolny'; Rate='-4'; Volume='100'; PitchMean='105'; PitchStd='14' },
    @{ Token='WPFestival.Szybki'; Name='WP Festival - Szybki'; Rate='4'; Volume='100'; PitchMean='105'; PitchStd='14' },
    @{ Token='WPFestival.Miekki'; Name='WP Festival - Miekki'; Rate='-1'; Volume='92'; PitchMean='98'; PitchStd='10' },
    @{ Token='WPFestival.Mocny'; Name='WP Festival - Mocny'; Rate='1'; Volume='118'; PitchMean='110'; PitchStd='18' }
)

Get-ChildItem 'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force

$engineClsid = '{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}'
$uiClsid = '{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}'

Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath $x64Dll -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Engine'
Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath $x86Dll -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Engine'
Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath $x64Dll -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 UI'
Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath $x86Dll -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 UI'

foreach ($preset in $presets) {
    Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -Preset $preset
    Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -Preset $preset
}
