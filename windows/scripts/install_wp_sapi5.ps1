param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'WP Festival SAPI5'),
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

$buildX64 = Join-Path $BuildRoot 'x64\Release\WPFestivalSapi5.dll'
$buildX86 = Join-Path $BuildRoot 'x86\Release\WPFestivalSapi5.dll'
$configExe = Join-Path $BuildRoot 'x64\Release\WPFestivalSapi5Config.exe'
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
        [Parameter(Mandatory = $true)][string]$TokenName,
        [Parameter(Mandatory = $true)][string]$VoiceName,
        [Parameter(Mandatory = $true)][string]$BaseRateAdjust,
        [Parameter(Mandatory = $true)][string]$BaseVolume,
        [Parameter(Mandatory = $true)][string]$PitchMean,
        [Parameter(Mandatory = $true)][string]$PitchStd
    )
    $clsid = '{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}'
    $uiClsid = '{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}'
    $tokenKey = Join-Path $BaseKey $tokenName
    $attrKey = Join-Path $tokenKey 'Attributes'
    $uiKey = Join-Path $tokenKey 'UI'
    $enginePropertiesKey = Join-Path $uiKey 'EngineProperties'
    New-Item -Path $attrKey -Force | Out-Null
    New-Item -Path $enginePropertiesKey -Force | Out-Null
    Set-Item -Path $tokenKey -Value $voiceName
    New-ItemProperty -Path $tokenKey -Name 'CLSID' -Value $clsid -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name '409' -Value 'WP Festival' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseRateAdjust' -Value $BaseRateAdjust -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseVolume' -Value $BaseVolume -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'PitchMean' -Value $PitchMean -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'PitchStd' -Value $PitchStd -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Name' -Value 'WP Festival' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Vendor' -Value 'WP reverse-engineered runtime' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Language' -Value '0415' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Gender' -Value 'Male' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Age' -Value 'Adult' -PropertyType String -Force | Out-Null
    Set-Item -Path $uiKey -Value ''
    Set-Item -Path $enginePropertiesKey -Value ''
    New-ItemProperty -Path $enginePropertiesKey -Name 'CLSID' -Value $uiClsid -PropertyType String -Force | Out-Null
}

$x64Dir = Join-Path $InstallDir 'x64'
$x86Dir = Join-Path $InstallDir 'x86'
New-Item -ItemType Directory -Force -Path $x64Dir | Out-Null
New-Item -ItemType Directory -Force -Path $x86Dir | Out-Null

Copy-Item $buildX64 (Join-Path $x64Dir 'WPFestivalSapi5.dll') -Force
Copy-Item $buildX86 (Join-Path $x86Dir 'WPFestivalSapi5.dll') -Force
Copy-Item $configExe (Join-Path $InstallDir 'WPFestivalSapi5Config.exe') -Force
Copy-Item $festivalExe (Join-Path $x64Dir 'festival.exe') -Force
Copy-Item $festivalExe (Join-Path $x86Dir 'festival.exe') -Force

if (Test-Path (Join-Path $x64Dir 'wp_runtime_lib')) { Remove-Item -Recurse -Force (Join-Path $x64Dir 'wp_runtime_lib') }
if (Test-Path (Join-Path $x86Dir 'wp_runtime_lib')) { Remove-Item -Recurse -Force (Join-Path $x86Dir 'wp_runtime_lib') }
Copy-Item $runtimeLib (Join-Path $x64Dir 'wp_runtime_lib') -Recurse -Force
Copy-Item $runtimeLib (Join-Path $x86Dir 'wp_runtime_lib') -Recurse -Force

powershell -ExecutionPolicy Bypass -File $registerScript -DllPath (Join-Path $x64Dir 'WPFestivalSapi5.dll')
& "$env:WINDIR\SysWOW64\WindowsPowerShell\v1.0\powershell.exe" -ExecutionPolicy Bypass -File $registerScript -DllPath (Join-Path $x86Dir 'WPFestivalSapi5.dll')

$presets = @(
    @{ Token='WPFestival.Standard'; Name='WP Festival (SAPI5)'; Rate='0'; Volume='100'; PitchMean='105'; PitchStd='14' },
    @{ Token='WPFestival.Gleboki'; Name='WP Festival - Gleboki'; Rate='-1'; Volume='108'; PitchMean='92'; PitchStd='12' },
    @{ Token='WPFestival.Jasny'; Name='WP Festival - Jasny'; Rate='0'; Volume='100'; PitchMean='122'; PitchStd='16' },
    @{ Token='WPFestival.Wolny'; Name='WP Festival - Wolny'; Rate='-4'; Volume='100'; PitchMean='105'; PitchStd='14' },
    @{ Token='WPFestival.Szybki'; Name='WP Festival - Szybki'; Rate='4'; Volume='100'; PitchMean='105'; PitchStd='14' },
    @{ Token='WPFestival.Miekki'; Name='WP Festival - Miekki'; Rate='-1'; Volume='92'; PitchMean='98'; PitchStd='10' },
    @{ Token='WPFestival.Mocny'; Name='WP Festival - Mocny'; Rate='1'; Volume='118'; PitchMean='110'; PitchStd='18' }
)

$userTokenRoots = @(
    'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens',
    'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens'
)

if ($MachineWide) {
    foreach ($rootKey in $userTokenRoots) {
        Get-ChildItem $rootKey -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
    }
} else {
    foreach ($rootKey in $userTokenRoots) {
        foreach ($preset in $presets) {
            Set-VoiceToken -BaseKey $rootKey -TokenName $preset.Token -VoiceName $preset.Name -BaseRateAdjust $preset.Rate -BaseVolume $preset.Volume -PitchMean $preset.PitchMean -PitchStd $preset.PitchStd
        }
    }
}

if ($MachineWide) {
    $engineClsid = '{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}'
    $uiClsid = '{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath (Join-Path $x64Dir 'WPFestivalSapi5.dll') -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Engine'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath (Join-Path $x86Dir 'WPFestivalSapi5.dll') -Clsid $engineClsid -DisplayName 'WP Festival SAPI5 Engine'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\CLSID' -DllPath (Join-Path $x64Dir 'WPFestivalSapi5.dll') -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 UI'
    Set-ComRegistration -BaseKey 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID' -DllPath (Join-Path $x86Dir 'WPFestivalSapi5.dll') -Clsid $uiClsid -DisplayName 'WP Festival SAPI5 UI'
    foreach ($preset in $presets) {
        Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -TokenName $preset.Token -VoiceName $preset.Name -BaseRateAdjust $preset.Rate -BaseVolume $preset.Volume -PitchMean $preset.PitchMean -PitchStd $preset.PitchStd
        Set-VoiceToken -BaseKey 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -TokenName $preset.Token -VoiceName $preset.Name -BaseRateAdjust $preset.Rate -BaseVolume $preset.Volume -PitchMean $preset.PitchMean -PitchStd $preset.PitchStd
    }
}

[pscustomobject]@{
    InstallDir = $InstallDir
    X64Dll = (Join-Path $x64Dir 'WPFestivalSapi5.dll')
    X86Dll = (Join-Path $x86Dir 'WPFestivalSapi5.dll')
    ConfigExe = (Join-Path $InstallDir 'WPFestivalSapi5Config.exe')
    Token = 'WPFestival.Standard'
    MachineWide = [bool]$MachineWide
    Voices = ($presets | ForEach-Object { $_.Name }) -join '; '
}
