param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'WP Festival SAPI5 Female'),
    [switch]$MachineWide
)

$ErrorActionPreference = 'SilentlyContinue'

$x64Dll = Join-Path $InstallDir 'x64\WPFestivalSapi5Female.dll'
$x86Dll = Join-Path $InstallDir 'x86\WPFestivalSapi5Female.dll'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$unregisterScript = Join-Path $root 'unregister_wp_com.ps1'

if (Test-Path $x64Dll) {
    powershell -ExecutionPolicy Bypass -File $unregisterScript -DllPath $x64Dll
}
if (Test-Path $x86Dll) {
    & "$env:WINDIR\SysWOW64\WindowsPowerShell\v1.0\powershell.exe" -ExecutionPolicy Bypass -File $unregisterScript -DllPath $x86Dll
}

Get-ChildItem 'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force

if ($MachineWide) {
    Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
    Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{22B4CE69-FA37-4A16-93DE-04168FD73A87}' -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{22B4CE69-FA37-4A16-93DE-04168FD73A87}' -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{51EC7519-9E37-4712-B83F-28E99342C49C}' -Recurse -Force
    Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{51EC7519-9E37-4712-B83F-28E99342C49C}' -Recurse -Force
}

if (Test-Path $InstallDir) {
    Remove-Item $InstallDir -Recurse -Force
}
