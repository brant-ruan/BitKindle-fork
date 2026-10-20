#requires -Version 5.1
param([string]$Executable = (Join-Path $PSScriptRoot 'build/mario.exe'))
$ErrorActionPreference = 'Stop'
$Executable = [IO.Path]::GetFullPath($Executable)
if (!(Test-Path -LiteralPath $Executable)) {
    throw 'Build mario first'
}
$bytes = [IO.File]::ReadAllBytes($Executable)
$pe = [BitConverter]::ToInt32($bytes, 0x3c)
if ([BitConverter]::ToUInt16($bytes, $pe + 24 + 68) -ne 2) {
    throw "Expected GUI subsystem: $Executable"
}
foreach ($mode in '--self-test', '--smoke-test', '--demo-test') {
    $arguments = $mode
    $timeout = 30000
    if ($mode -eq '--demo-test') {
        $report = Join-Path (Split-Path $Executable) 'gameplay-test.txt'
        $arguments += ' "' + $report + '"'
        $timeout = 120000
    }
    $p = Start-Process -FilePath $Executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
    [void]$p.Handle
    try {
        if (!$p.WaitForExit($timeout)) {
            $p.Kill()
            throw "Timed out: $mode"
        }
        $p.WaitForExit()
        if ($p.ExitCode -ne 0) {
            throw "$mode failed: $($p.ExitCode)"
        }
        "PASS $([IO.Path]::GetFileName($Executable)) $mode"
    }
    finally {
        $p.Dispose()
    }
}
