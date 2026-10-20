param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
foreach ($script in 'test.ps1', 'test-matrix.ps1', 'test-diagnostics.ps1') {
    & (Join-Path $PSScriptRoot $script) -BuildDirectory $BuildDirectory
    if (!$?) {
        throw "Diagnostic gate failed: $script"
    }
}
& (Join-Path $PSScriptRoot 'test-diagnostics.ps1') -BuildDirectory $BuildDirectory -ObjectMode
if (!$?) {
    throw 'Object diagnostic gate failed'
}
& (Join-Path $PSScriptRoot 'test-skipped.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Skipped directive direct tests failed'
}
& (Join-Path $PSScriptRoot 'test-skipped.ps1') -BuildDirectory $BuildDirectory -ObjectMode
if (!$?) {
    throw 'Skipped directive object tests failed'
}
'PASS all C89 diagnostic and preprocessing gates'