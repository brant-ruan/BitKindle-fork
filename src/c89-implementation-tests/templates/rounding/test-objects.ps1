param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath($Snapshot)
$build = [IO.Path]::GetFullPath($BuildDirectory)
. (Join-Path $snapshot 'tools/compile.ps1')
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (Join-Path $snapshot 'include') + ';' + (Join-Path $snapshot 'base/include')
    $options = ((Get-Content (Join-Path $snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        $obj = Join-Path $PSScriptRoot "object-probe$g.c1o"
        $exe = Join-Path $PSScriptRoot "object-probe$g.exe"
        Compile (Join-Path $build "host/c1-stage$g.exe") (Join-Path $PSScriptRoot 'probe.c') $obj
        $runtime = Join-Path $build "namespace/runtime-policy$g/runtime.c1o"
        $p = Start-Process -FilePath (Join-Path $build "link-stage$g.exe") -ArgumentList ('"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options) -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Link timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw 'Link failed'
        }
        $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Run timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw "Rounding conversion probe returned $($p.ExitCode)"
        }
        "PASS generation $g default rounding conversions"
    }
    foreach ($ext in 'c1o', 'exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "object-probe1.$ext"))
        $b = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "object-probe2.$ext"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Artifact length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw 'Artifact mismatch'
            }
        }
    }
    'PASS both generations rounding object review and two equal artifact pairs'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
