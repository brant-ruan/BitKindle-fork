param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
$build = $BuildDirectory
. "$snapshot/tools/compile.ps1"
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';'
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        $obj = "$PSScriptRoot/fenv-scope-probe$g.c1o"
        $exe = "$PSScriptRoot/fenv-scope-probe$g.exe"
        Compile "$BuildDirectory/host/c1-stage$g.exe" "$PSScriptRoot/fenv-scope-probe.c" $obj
        & "$build/link-stage$g.exe" $exe __c1_start $obj "$BuildDirectory/host/runtime0.c1o" ($options -split ' ')
        if ($LASTEXITCODE) {
            throw 'Link failed'
        }
        & $exe
        if ($LASTEXITCODE) {
            throw "Contract probe failed: $LASTEXITCODE"
        }
    }
    foreach ($ext in 'c1o', 'exe') {
        if ((Get-FileHash "$PSScriptRoot/fenv-scope-probe1.$ext").Hash -ne (Get-FileHash "$PSScriptRoot/fenv-scope-probe2.$ext").Hash) {
            throw 'Artifacts differ'
        }
    }
    'PASS two C99 FENV_ACCESS scope executions and equal object/executable pairs'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}

