param([Parameter(Mandatory = $true)][string]$BaseBuild, [Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$BaseBuild = [IO.Path]::GetFullPath($BaseBuild)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
. (Join-Path $PSScriptRoot 'tools/compile.ps1')
function Same([string]$Left, [string]$Right) {
    $a = [IO.File]::ReadAllBytes($Left)
    $b = [IO.File]::ReadAllBytes($Right)
    if ($a.Length -ne $b.Length) {
        throw 'Length mismatch'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw "Byte mismatch: $Left"
        }
    }
    "PASS bytes $Left = $Right"
}
$savedInclude = $env:C1_INCLUDE
$savedOutput = $env:C1_OUTPUT
$savedStandard = $env:C1_STANDARD
$savedRuntime = $env:C1_RUNTIME
try {
    $env:C1_STANDARD = $null
    $env:C1_RUNTIME = $null
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = Join-Path $PSScriptRoot 'base/include'
    $previous = Join-Path $BaseBuild 'c1-stage2.exe'
    foreach ($kind in @('identifier', 'object')) {
        $dir = Join-Path $BuildDirectory $kind
        New-Item -ItemType Directory -Force $dir | Out-Null
        for ($g = 0; $g -le 2; $g++) {
            $exe = Join-Path $dir ('c1-stage' + $g + '.exe')
            Compile $previous (Join-Path $PSScriptRoot ($kind + '/compiler.c')) $exe
            $previous = $exe
        }
        Same (Join-Path $dir 'c1-stage1.exe') (Join-Path $dir 'c1-stage2.exe')
    }
    foreach ($g in 1, 2) {
        Compile (Join-Path $BuildDirectory ('object/c1-stage' + $g + '.exe')) (Join-Path $PSScriptRoot 'linker/link.c') (Join-Path $BuildDirectory ('link-stage' + $g + '.exe'))
    }
    Same (Join-Path $BuildDirectory 'link-stage1.exe') (Join-Path $BuildDirectory 'link-stage2.exe')
    $apis = Get-Content (Join-Path $PSScriptRoot 'runtime/imports.json') -Raw | ConvertFrom-Json
    $options = ($apis | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    $env:C1_INCLUDE = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $PSScriptRoot $_ }) -join ';'
    $dir = Join-Path $BuildDirectory 'host'
    New-Item -ItemType Directory -Force $dir | Out-Null
    for ($g = 0; $g -le 2; $g++) {
        $obj = Join-Path $dir ('compiler' + $g + '.c1o')
        $rt = Join-Path $dir ('runtime' + $g + '.c1o')
        $exe = Join-Path $dir ('c1-stage' + $g + '.exe')
        $env:C1_OUTPUT = 'object'
        Compile $previous (Join-Path $PSScriptRoot 'host/compiler.c') $obj
        $runtimeSource = if ($g -eq 0) {
            'runtime/bootstrap-runtime.c'
        }
        else {
            'runtime/runtime.c'
        }
        Compile $previous (Join-Path $PSScriptRoot $runtimeSource) $rt
        $env:C1_OUTPUT = $null
        $p = Start-Process -FilePath (Join-Path $BuildDirectory 'link-stage2.exe') -ArgumentList ('"' + $exe + '" __c1_start "' + $obj + '" "' + $rt + '" ' + $options) -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (-not $p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Link timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw 'Host compiler link failed'
        }
        $previous = $exe
    }
    foreach ($stem in @('compiler', 'runtime', 'c1-stage')) {
        $ext = if ($stem -eq 'c1-stage') {
            '.exe'
        }
        else {
            '.c1o'
        }
        Same (Join-Path $dir ($stem + '1' + $ext)) (Join-Path $dir ($stem + '2' + $ext))
    }
    if ($SkipTests) {
        'BUILT final toolchain; integration tests skipped'
        return 
    }
    foreach ($g in 1, 2) {
        foreach ($script in @('build-runtime.ps1', 'test-runtime.ps1', 'test.ps1', 'test-imports.ps1', 'test-subsystem.ps1')) {
            & (Join-Path $PSScriptRoot ('integration/' + $script)) -Generation $g -BuildDirectory $BuildDirectory
            if (-not $?) {
                throw "Integration failed: $script generation $g"
            }
        }
    }
    'PASS object toolchain self-compilation and both generations integration'
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $env:C1_OUTPUT = $savedOutput
    $env:C1_STANDARD = $savedStandard
    $env:C1_RUNTIME = $savedRuntime
}
