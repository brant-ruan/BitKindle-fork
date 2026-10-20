param([string]$Snapshot = 'src/bootstrap-c99', [string]$BuildDirectory = 'src/bootstrap-c99/build-from-seed', [string]$ScratchDirectory = (Join-Path $PSScriptRoot 'build'))
$ErrorActionPreference = 'Stop'
$Snapshot = [IO.Path]::GetFullPath($Snapshot)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
if (Test-Path $ScratchDirectory) {
    throw 'Use a new scratch directory'
}
New-Item -ItemType Directory $ScratchDirectory | Out-Null
. (Join-Path $Snapshot 'tools/compile.ps1')
$template = Get-Content (Join-Path $PSScriptRoot 'probe.c') -Raw
$options = ((Get-Content (Join-Path $Snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
$cases = @()
foreach ($sig in 2, 4, 8, 11, 15, 22) {
    foreach ($mode in 0, 1, 2) {
        $cases += @{name = "signal-$sig-$mode"
            sig = $sig
            mode = $mode
            exit = $(if ($mode) {
                    0
                }
                elseif ($sig -eq 22) {
                    3
                }
                else {
                    128 + $sig
                })
        }
    }
}
foreach ($mode in 3, 4, 5) {
    $cases += @{name = "abort-$mode"
        sig = 22
        mode = $mode
        exit = 3
    }
}
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (Join-Path $Snapshot 'include') + ';' + (Join-Path $Snapshot 'base/include')
    foreach ($case in $cases) {
        $source = Join-Path $ScratchDirectory ($case.name + '.c')
        [IO.File]::WriteAllText($source, "#define TEST_SIGNAL $($case.sig)`n#define TEST_MODE $($case.mode)`n" + $template, [Text.UTF8Encoding]::new($false))
        foreach ($g in 1, 2) {
            $stem = Join-Path $ScratchDirectory ($case.name + "-$g")
            Compile (Join-Path $BuildDirectory "host/c1-stage$g.exe") $source ($stem + '.c1o')
            $runtime = Join-Path $BuildDirectory "namespace/runtime-policy$g/runtime.c1o"
            $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList ('"' + $stem + '.exe" __c1_start "' + $stem + '.c1o" "' + $runtime + '" ' + $options) -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Link timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw 'Link failed'
            }
            $p = Start-Process -FilePath ($stem + '.exe') -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Run timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode -ne $case.exit) {
                throw "Unexpected exit $($p.ExitCode): $($case.name)"
            }
            "PASS generation $g $($case.name) exit $($case.exit)"
        }
        foreach ($ext in 'exe', 'c1o') {
            $a = [IO.File]::ReadAllBytes((Join-Path $ScratchDirectory ($case.name + "-1.$ext")))
            $b = [IO.File]::ReadAllBytes((Join-Path $ScratchDirectory ($case.name + "-2.$ext")))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Empty or unequal pair'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Unequal artifact bytes'
                }
            }
        }
    }
    'PASS 42 signal executions and 42 nonempty equal artifact pairs'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
