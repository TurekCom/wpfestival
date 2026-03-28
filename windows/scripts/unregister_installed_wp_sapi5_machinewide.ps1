param(
    [string]$InstallDir = $PSScriptRoot
)

$ErrorActionPreference = 'SilentlyContinue'

Get-ChildItem 'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestival.*' } | Remove-Item -Recurse -Force

Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}' -Recurse -Force
Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{7F5F0B2E-4C3F-4A3D-9774-91D6D603D468}' -Recurse -Force
Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}' -Recurse -Force
Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{8C37E6AD-A71D-4EF3-88A3-57D3D5E17A4C}' -Recurse -Force
