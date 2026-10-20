param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
$tools = if ($ToolDirectory) {
    [IO.Path]::GetFullPath($ToolDirectory)
}
else {
    $build
}
$testRoot = Join-Path $build 'namespace/ucn'
New-Item -ItemType Directory -Force $testRoot | Out-Null
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'")).Replace("'^C1 error: '", "'(?m)^C1 error: '")
$cases = Get-Content (Join-Path $PSScriptRoot 'ucn/cases.json') -Raw | ConvertFrom-Json
$commands = foreach ($c in $cases) {
    "Check '$($c.name)' '" + $c.source.Replace("'", "''") + "' 0 " + $(if ($c.reject) {
            '$true'
        }
        else {
            '$false'
        })
}
$selected = $prefix + ($commands -join "`n") + "`n" + '"PASS $script:passed packaged UCN cases"'
[IO.File]::WriteAllText((Join-Path $testRoot 'selected.ps1'), $selected)
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($mode in 'c89', 'c99') {
        $env:C1_STANDARD = $mode
        foreach ($g in 1, 2) {
            & ([scriptblock]::Create($selected)) -Compiler "$tools/host/c1-stage$g.exe" -Assembler "$tools/link-stage2.exe" -CompilerKind C1 -ScratchDirectory "$testRoot/$mode-$g"
            if (!$?) {
                throw 'UCN case suite failed'
            }
        }
    }
    . "$snapshot/tools/compile.ps1"
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = 'object'
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        foreach ($unit in 'caller', 'callee') {
            Compile "$tools/host/c1-stage$g.exe" "$snapshot/namespace/ucn/$unit.c" "$testRoot/$unit$g.c1o"
        }
        & "$tools/link-stage$g.exe" "$testRoot/cross$g.exe" __c1_start "$testRoot/caller$g.c1o" "$testRoot/callee$g.c1o" "$tools/host/runtime$g.c1o" ($options -split ' ')
        if ($LASTEXITCODE) {
            throw 'UCN cross-unit link failed'
        }
        $p = Start-Process -FilePath "$testRoot/cross$g.exe" -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Cross-unit timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw 'UCN cross-unit execution failed'
        }
        "PASS UCN cross-unit generation $g"
    }
    'PASS all packaged UCN executions and rejections'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
