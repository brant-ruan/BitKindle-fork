param([string]$Stage1, [string]$Stage2, [string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
# Compile is provided by the calling build-c1.ps1. Both tools were produced
# by the current from-seed chain; the host only transports their bytes.
$generatorSource = Join-Path $PSScriptRoot 'pi-table.c'
foreach ($generation in 1, 2) {
    $tool = if ($generation -eq 1) {
        $Stage1
    }
    else {
        $Stage2
    }
    $exe = Join-Path $BuildDirectory "pi-stage$generation.exe"
    Compile $tool $generatorSource $exe
    $header = Join-Path $BuildDirectory "two-over-pi-stage$generation.h"
    $process = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru -RedirectStandardOutput $header
    [void]$process.Handle
    if (-not $process.WaitForExit(120000)) {
        $process.Kill()
        throw 'Pi generator timeout'
    }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        throw 'Pi generator failed to certify constants'
    }
    $expected = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot '../lib/two-over-pi.h'))
    $actual = [IO.File]::ReadAllBytes($header)
    if ($expected.Length -ne $actual.Length) {
        throw 'Pi table size mismatch'
    }
    for ($i = 0; $i -lt $expected.Length; $i++) {
        if ($expected[$i] -ne $actual[$i]) {
            throw "Pi table mismatch at byte $i"
        }
    }
    "PASS generated pi table stage$generation equals source table"
}
$first = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory 'pi-stage1.exe'))
$second = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory 'pi-stage2.exe'))
if ($first.Length -ne $second.Length) {
    throw 'Pi generator size mismatch'
}
for ($i = 0; $i -lt $first.Length; $i++) {
    if ($first[$i] -ne $second[$i]) {
        throw "Pi generator mismatch at byte $i"
    }
}
'PASS pi generators byte-for-byte identical'
