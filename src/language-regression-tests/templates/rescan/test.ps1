param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath($Snapshot)
$build = [IO.Path]::GetFullPath($BuildDirectory)
. (Join-Path $snapshot 'tools/compile.ps1')
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = Join-Path $snapshot 'base/include'
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        $obj = Join-Path $PSScriptRoot "probe$g.c1o"
        $exe = Join-Path $PSScriptRoot "probe$g.exe"
        Compile (Join-Path $build "host/c1-stage$g.exe") (Join-Path $PSScriptRoot 'probe.c') $obj
        & (Join-Path $build "link-stage$g.exe") $exe __c1_start $obj (Join-Path $build "host/runtime$g.c1o") ($options -split ' ')
        if ($LASTEXITCODE) {
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
            throw "Probe generation $g failed: $($p.ExitCode)"
        }
        "PASS macro rescan probe generation $g"
    }
    foreach ($ext in 'c1o', 'exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "probe1.$ext"))
        $b = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "probe2.$ext"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw 'Bytes differ'
            }
        }
        "PASS identical $ext pair"
    }
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
