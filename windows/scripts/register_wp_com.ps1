param(
    [Parameter(Mandatory = $true)]
    [string]$DllPath
)

$ErrorActionPreference = 'Stop'
$resolvedDll = (Resolve-Path $DllPath).Path

$code = @"
using System;
using System.Runtime.InteropServices;
public static class NativeWpCom {
  [DllImport(@"$resolvedDll", ExactSpelling=true, PreserveSig=true)]
  public static extern int DllRegisterServer();
}
"@

Add-Type -TypeDefinition $code
$hr = [NativeWpCom]::DllRegisterServer()
if ($hr -ne 0) {
    throw ('DllRegisterServer failed: 0x{0:X8}' -f ([uint32]$hr))
}
