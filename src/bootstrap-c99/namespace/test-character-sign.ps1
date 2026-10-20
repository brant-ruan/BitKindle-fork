param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$ErrorActionPreference = 'Stop'
foreach ($name in 'main', 'raw') {
    & (Join-Path $PSScriptRoot "test-character-sign-$name.ps1") -BuildDirectory $BuildDirectory -ToolDirectory $ToolDirectory
    if (!$?) {
        throw "Character sign suite failed: $name"
    }
}
'PASS complete character sign suites'