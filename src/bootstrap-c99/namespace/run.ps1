param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
foreach ($name in 'test-runtime-policy.ps1', 'test-common-isolation.ps1', 'check-headers.ps1', 'audit-runtime-symbols.ps1') {
    & (Join-Path $PSScriptRoot $name) -BuildDirectory $BuildDirectory
    if (!$?) {
        throw "C89 namespace gate failed: $name"
    }
}
'PASS integrated C89 runtime isolation, common controls, public headers and export audit'