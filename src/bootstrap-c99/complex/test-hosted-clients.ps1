param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
$build = $BuildDirectory
. "$snapshot/tools/compile.ps1"
$out = Join-Path $PSScriptRoot 'hosted-clients'
New-Item -ItemType Directory -Force $out | Out-Null
$suites = @('basic', 'polar', 'sqrt', 'log', 'exp', 'pow', 'sincos', 'tan', 'atan', 'asin', 'special', 'multiply-env', 'divide-env', 'tgmath', 'real-inverse', 'fdim', 'hypot', 'remquo', 'cbrt', 'exp2', 'fma', 'erf', 'gamma', 'cx-pragma')
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = "$PSScriptRoot/include;" + ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        foreach ($suite in $suites) {
            Compile "$BuildDirectory/host/c1-stage$g.exe" "$PSScriptRoot/$suite-client.c" "$out/$suite$g.c1o"
            & "$build/link-stage$g.exe" "$out/$suite$g.exe" __c1_start "$out/$suite$g.c1o" "$BuildDirectory/host/runtime$g.c1o" ($options -split ' ')
            if ($LASTEXITCODE) {
                throw "Hosted link failed: $suite generation $g"
            }
            & "$out/$suite$g.exe"
            if ($LASTEXITCODE) {
                throw "Hosted execution failed: $suite generation $g exit $LASTEXITCODE"
            }
            "PASS hosted self-compiled client $suite generation $g"
        }
    }
    foreach ($suite in $suites) {
        foreach ($ext in 'c1o', 'exe') {
            $a = [IO.File]::ReadAllBytes("$out/$suite`1.$ext")
            $b = [IO.File]::ReadAllBytes("$out/$suite`2.$ext")
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Hosted client length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Hosted client byte mismatch'
                }
            }
        }
    }
    "PASS $($suites.Count*2) hosted self-compiled client executions, $($suites.Count) equal object pairs and $($suites.Count) equal executable pairs"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
