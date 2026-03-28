param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'WP Festival SAPI5 Female'),
    [string]$BuildRoot,
    [string]$FestivalExe,
    [string]$RuntimeLib,
    [switch]$MachineWide
)

$ErrorActionPreference = 'Stop'

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$windowsRoot = Split-Path -Parent $scriptRoot
$repoRoot = Split-Path -Parent $windowsRoot
if (-not $BuildRoot) { $BuildRoot = Join-Path $windowsRoot 'build' }
if (-not $FestivalExe) { $FestivalExe = Join-Path $repoRoot 'runtime\festival.exe' }
if (-not $RuntimeLib) { $RuntimeLib = Join-Path $repoRoot 'runtime\wp_runtime_lib' }

$buildX64 = Join-Path $BuildRoot 'x64\Release\WPFestivalSapi5Female.dll'
$buildX86 = Join-Path $BuildRoot 'x86\Release\WPFestivalSapi5Female.dll'
$configExe = Join-Path $BuildRoot 'x64\Release\WPFestivalSapi5FemaleConfig.exe'
$registerScript = Join-Path $scriptRoot 'register_wp_com.ps1'

if (!(Test-Path $buildX64)) { throw "Brak DLL x64: $buildX64" }
if (!(Test-Path $buildX86)) { throw "Brak DLL x86: $buildX86" }
if (!(Test-Path $configExe)) { throw "Brak konfiguratora: $configExe" }
if (!(Test-Path $festivalExe)) { throw "Brak festival.exe: $festivalExe" }
if (!(Test-Path $runtimeLib)) { throw "Brak wp_runtime_lib: $runtimeLib" }
if (!(Test-Path $registerScript)) { throw "Brak skryptu rejestracji COM: $registerScript" }

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

$x64Dir = Join-Path $InstallDir 'x64'
$x86Dir = Join-Path $InstallDir 'x86'
New-Item -ItemType Directory -Force -Path $x64Dir | Out-Null
New-Item -ItemType Directory -Force -Path $x86Dir | Out-Null

Copy-Item $buildX64 (Join-Path $x64Dir 'WPFestivalSapi5Female.dll') -Force
Copy-Item $buildX86 (Join-Path $x86Dir 'WPFestivalSapi5Female.dll') -Force
Copy-Item $configExe (Join-Path $InstallDir 'WPFestivalSapi5FemaleConfig.exe') -Force
Copy-Item $festivalExe (Join-Path $x64Dir 'festival.exe') -Force
Copy-Item $festivalExe (Join-Path $x86Dir 'festival.exe') -Force

if (Test-Path (Join-Path $x64Dir 'wp_runtime_lib')) { Remove-Item -Recurse -Force (Join-Path $x64Dir 'wp_runtime_lib') }
if (Test-Path (Join-Path $x86Dir 'wp_runtime_lib')) { Remove-Item -Recurse -Force (Join-Path $x86Dir 'wp_runtime_lib') }
Copy-Item $runtimeLib (Join-Path $x64Dir 'wp_runtime_lib') -Recurse -Force
Copy-Item $runtimeLib (Join-Path $x86Dir 'wp_runtime_lib') -Recurse -Force

powershell -ExecutionPolicy Bypass -File $registerScript -DllPath (Join-Path $x64Dir 'WPFestivalSapi5Female.dll')
& "$env:WINDIR\SysWOW64\WindowsPowerShell\v1.0\powershell.exe" -ExecutionPolicy Bypass -File $registerScript -DllPath (Join-Path $x86Dir 'WPFestivalSapi5Female.dll')

$presets = @(
    @{ Token='WPFestivalFemale.Standard'; Name='WP Festival Zenski (SAPI5)'; Rate='0'; Volume='100'; PitchMean='145'; PitchStd='16' },
    @{ Token='WPFestivalFemale.Gleboki'; Name='WP Festival Zenski - Gleboki'; Rate='-1'; Volume='106'; PitchMean='136'; PitchStd='14' },
    @{ Token='WPFestivalFemale.Jasny'; Name='WP Festival Zenski - Jasny'; Rate='0'; Volume='100'; PitchMean='156'; PitchStd='18' },
    @{ Token='WPFestivalFemale.Wolny'; Name='WP Festival Zenski - Wolny'; Rate='-4'; Volume='100'; PitchMean='145'; PitchStd='16' },
    @{ Token='WPFestivalFemale.Szybki'; Name='WP Festival Zenski - Szybki'; Rate='4'; Volume='100'; PitchMean='145'; PitchStd='16' },
    @{ Token='WPFestivalFemale.Miekki'; Name='WP Festival Zenski - Miekki'; Rate='-1'; Volume='94'; PitchMean='141'; PitchStd='14' },
    @{ Token='WPFestivalFemale.Mocny'; Name='WP Festival Zenski - Mocny'; Rate='1'; Volume='116'; PitchMean='148'; PitchStd='19' }
)

$userTokenRoots = @(
    'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens',
    'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens'
)

if ($MachineWide) {
    foreach ($rootKey in $userTokenRoots) {
        Get-ChildItem $rootKey -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
    }
} else {
    foreach ($rootKey in $userTokenRoots) {
        foreach ($preset in $presets) {
            Set-VoiceToken -BaseKey $rootKey -Preset $preset
        }
    }
}

if ($MachineWide) {
    $engineClsid = '{22B4CE69-FA37-4A16-93DE-04168FD73A87}'
    $uiClsid = '{51EC7519-9E37-4712-B83F-28E99342C49C}'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath (Join-Path $x64Dir 'WPFestivalSapi5Female.dll') -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Female Engine'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath (Join-Path $x86Dir 'WPFestivalSapi5Female.dll') -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Female Engine'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath (Join-Path $x64Dir 'WPFestivalSapi5Female.dll') -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 Female UI'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath (Join-Path $x86Dir 'WPFestivalSapi5Female.dll') -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 Female UI'
    foreach ($preset in $presets) {
        Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -Preset $preset
        Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -Preset $preset
    }
}

[pscustomobject]@{
    InstallDir = $InstallDir
    X64Dll = (Join-Path $x64Dir 'WPFestivalSapi5Female.dll')
    X86Dll = (Join-Path $x86Dir 'WPFestivalSapi5Female.dll')
    ConfigExe = (Join-Path $InstallDir 'WPFestivalSapi5FemaleConfig.exe')
    Token = 'WPFestivalFemale.Standard'
    MachineWide = [bool]$MachineWide
    Voices = ($presets | ForEach-Object { $_.Name }) -join '; '
}
