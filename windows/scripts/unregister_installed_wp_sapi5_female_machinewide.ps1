param(
    [string]$InstallDir = $PSScriptRoot
)

$ErrorActionPreference = 'SilentlyContinue'

Get-ChildItem 'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force
Get-ChildItem 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens' -ErrorAction SilentlyContinue | Where-Object { $_.PSChildName -like 'WPFestivalFemale.*' } | Remove-Item -Recurse -Force

Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{22B4CE69-FA37-4A16-93DE-04168FD73A87}' -Recurse -Force
Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{22B4CE69-FA37-4A16-93DE-04168FD73A87}' -Recurse -Force
Remove-Item 'HKLM:\SOFTWARE\Classes\CLSID\{51EC7519-9E37-4712-B83F-28E99342C49C}' -Recurse -Force
Remove-Item 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\{51EC7519-9E37-4712-B83F-28E99342C49C}' -Recurse -Force
