param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath($Snapshot)
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = $null
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (Join-Path $snapshot 'include') + ';' + (Join-Path $snapshot 'base/include')
    foreach ($g in 1, 2) {
        $exe = Join-Path $PSScriptRoot "probe$g.exe"
        $p = Start-Process -FilePath (Join-Path $build "host/c1-stage$g.exe") -WindowStyle Hidden -PassThru -RedirectStandardInput (Join-Path $PSScriptRoot 'probe.c') -RedirectStandardOutput $exe -RedirectStandardError (Join-Path $PSScriptRoot "probe$g.stderr")
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Compile timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw 'Probe compilation failed'
        }
        $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Run timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw "Probe returned $($p.ExitCode)"
        }
        "PASS generation $g character constants and escape mappings"
    }
    $a = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'probe1.exe'))
    $b = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'probe2.exe'))
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw 'Artifact length mismatch'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Artifact byte mismatch'
        }
    }
    'PASS equal character mapping probe artifacts'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
