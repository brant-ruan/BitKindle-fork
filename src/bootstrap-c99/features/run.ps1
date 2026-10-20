param([Parameter(Mandatory = $true)][string]$BuildDirectory, [Parameter(Mandatory = $true)][string]$Assembler)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$saved = $env:C1_OUTPUT
try {
    $env:C1_OUTPUT = $null
    foreach ($g in 1, 2) {
        & (Join-Path $PSScriptRoot 'check.ps1') -Compiler (Join-Path $BuildDirectory "host/c1-stage$g.exe") -Assembler $Assembler -CompilerKind C1 -ScratchDirectory (Join-Path $BuildDirectory "features/language$g")
        if (!$?) {
            throw "Language features failed in generation $g"
        }
    }
    foreach ($file in 'inline/test-objects.ps1', 'inline/test-link-edges.ps1', 'compound/test-objects.ps1') {
        & (Join-Path $PSScriptRoot $file) -BuildDirectory $BuildDirectory
        if (!$?) {
            throw "Feature linking failed: $file"
        }
    }
    'PASS both generations core language and linking features'
}
finally {
    $env:C1_OUTPUT = $saved
}
