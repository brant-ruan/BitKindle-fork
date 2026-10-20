param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
if (!$ToolDirectory) {
    $ToolDirectory = $build
}
$toolBuild = [IO.Path]::GetFullPath($ToolDirectory)
$dir = Join-Path $build 'namespace/math-policy'
New-Item -ItemType Directory -Force $dir | Out-Null
. (Join-Path $snapshot 'tools/compile.ps1')
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';'
    $options = ((Get-Content (Join-Path $snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($mode in 'c89', 'c99') {
        foreach ($g in 1, 2) {
            $env:C1_STANDARD = $mode
            $env:C1_RUNTIME = $null
            $obj = Join-Path $dir "paired-$mode-$g.c1o"
            $exe = Join-Path $dir "paired-$mode-$g.exe"
            Compile (Join-Path $toolBuild "host/c1-stage$g.exe") (Join-Path $PSScriptRoot 'math-policy.c') $obj
            $runtime = Join-Path $dir "paired-runtime-$mode-$g.c1o"
            $env:C1_STANDARD = $null
            $env:C1_RUNTIME = if ($mode -eq 'c89') {
                'c89'
            }
            else {
                $null
            }
            Compile (Join-Path $toolBuild "host/c1-stage$g.exe") (Join-Path $snapshot "runtime/runtime-$mode.c") $runtime
            $env:C1_STANDARD = $mode
            $env:C1_RUNTIME = $null
            $p = Start-Process -FilePath (Join-Path $toolBuild "link-stage$g.exe") -ArgumentList ('"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options) -WindowStyle Hidden -PassThru
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
                throw "Math policy probe returned $($p.ExitCode)"
            }
            "PASS generation $g mode $mode math range policy"
        }
        foreach ($ext in 'c1o', 'exe') {
            $a = [IO.File]::ReadAllBytes((Join-Path $dir "paired-$mode-1.$ext"))
            $b = [IO.File]::ReadAllBytes((Join-Path $dir "paired-$mode-2.$ext"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Artifact length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Artifact mismatch'
                }
            }
        }
    }
    'PASS C89/C99 paired math range policy, four executions and four equal artifact pairs'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}





