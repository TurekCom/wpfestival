param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'WP Festival SAPI5'),
    [switch]$MachineWide
)

$ErrorActionPreference = 'SilentlyContinue'

$x64Dll = Join-Path $InstallDir 'x64\WPFestivalSapi5.dll'
$x86Dll = Join-Path $InstallDir 'x86\WPFestivalSapi5.dll'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$unregisterScript = Join-Path $root 'unregister_wp_com.ps1'

if (Test-Path $x64Dll) {
    powershell -ExecutionPolicy Bypass -File $unregisterScript -DllPath $x64Dll
}
if (Test-Path $x86Dll) {
    & "$env:WINDIR\SysWOW64\WindowsPowerShell\v1.0\powershell.exe" -ExecutionPolicy Bypass -File $unregisterScript -DllPath $x86Dll
}

Get-ChildItem 'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force

if ($MachineWide) {
    Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
    Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}' -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}' -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}' -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}' -Recurse -Force
}

if (Test-Path $InstallDir) {
    Remove-Item $InstallDir -Recurse -Force
}
