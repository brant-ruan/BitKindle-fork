#requires -Version 5.1
param([int]$Generation = 2, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$root = Join-Path $BuildDirectory "integration/subsystem$Generation"
New-Item -ItemType Directory -Force $root | Out-Null
. (Join-Path $PSScriptRoot '../tools/compile.ps1')
$linker = Join-Path $BuildDirectory "link-stage$Generation.exe"
$saved = @{}
foreach ($name in 'C1_OUTPUT', 'C1_STANDARD', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
function Run([string]$File, [string[]]$Arguments, [int]$Expected) {
    $argText = ConvertTo-C1WindowsArguments $Arguments
    $p = Start-Process -FilePath $File -ArgumentList $argText -WindowStyle Hidden -PassThru
    [void]$p.Handle
    try {
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Subsystem test timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode -ne $Expected) {
            throw "Exit $($p.ExitCode), expected $Expected : $File"
        }
    }
    finally {
        $p.Dispose()
    }
}
try {
    $env:C1_OUTPUT = 'object'
    $env:C1_STANDARD = 'c99'
    $env:C1_RUNTIME = $null
    $source = Join-Path $root 'probe.c'
    $obj = Join-Path $root 'probe.c1o'
    [IO.File]::WriteAllText($source, 'void *GetConsoleWindow(void); int main(void){return GetConsoleWindow()?3:2;}', [Text.Encoding]::ASCII)
    Compile (Join-Path $BuildDirectory "host/c1-stage$Generation.exe") $source $obj
    foreach ($mode in 'default', 'console', 'windows') {
        $exe = Join-Path $root "$mode.exe"
        $args = @($exe, 'main', $obj, '--import', 'GetConsoleWindow', 'KERNEL32.dll', 'GetConsoleWindow')
        if ($mode -ne 'default') {
            $args += @('--subsystem', $mode)
        }
        Run $linker $args 0
        $bytes = [IO.File]::ReadAllBytes($exe)
        $pe = [BitConverter]::ToInt32($bytes, 0x3c)
        $field = $pe + 24 + 68
        $expected = if ($mode -eq 'windows') {
            2
        }
        else {
            3
        }
        if ([BitConverter]::ToUInt16($bytes, $field) -ne $expected) {
            throw 'Wrong PE subsystem'
        }
        Run $exe @('probe') $expected
    }
    $a = [IO.File]::ReadAllBytes((Join-Path $root 'default.exe'))
    $b = [IO.File]::ReadAllBytes((Join-Path $root 'console.exe'))
    $c = [IO.File]::ReadAllBytes((Join-Path $root 'windows.exe'))
    if ($a.Length -ne $b.Length -or $a.Length -ne $c.Length) {
        throw 'Unexpected image length change'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Default console output changed'
        }
        if ($i -ne $field -and $a[$i] -ne $c[$i]) {
            throw 'GUI altered another PE byte'
        }
    }
    $destination = Join-Path $root 'default.exe'
    $hash = (Get-FileHash $destination).Hash
    foreach ($bad in @(@('--subsystem'), @('--subsystem', 'invalid'), @('--subsystem', 'console', '--subsystem', 'windows'))) {
        Run $linker (@($destination, 'main', $obj) + $bad) 1
        if ((Get-FileHash $destination).Hash -ne $hash) {
            throw 'Invalid option overwrote output'
        }
    }
    'PASS console/GUI headers, actual console attachment, default identity, isolated PE change and invalid-option preservation'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
