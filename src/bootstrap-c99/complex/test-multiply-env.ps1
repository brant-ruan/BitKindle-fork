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
    $env:C1_INCLUDE = "$PSScriptRoot/include;" + ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        foreach ($unit in 'provider', 'client') {
            Compile "$BuildDirectory/host/c1-stage$g.exe" $(if ($unit -eq 'provider') {
                    "$PSScriptRoot/lib/basic.c"
                }
                else {
                    "$PSScriptRoot/multiply-env-client.c"
                }) "$PSScriptRoot/multiply-env-$unit$g.c1o"
        }
        foreach ($p in 1, 2) {
            $exe = "$PSScriptRoot/multiply-env-$g-$p.exe"
            # Build both compiler generations' providers before mixing generations.
            if ($p -gt $g) {
                continue
            }
            & "$build/link-stage$g.exe" $exe __c1_start "$PSScriptRoot/multiply-env-client$g.c1o" "$PSScriptRoot/multiply-env-provider$p.c1o" "$BuildDirectory/host/runtime0.c1o" ($options -split ' ')
            if ($LASTEXITCODE) {
                throw 'Basic complex library link failed'
            }
            & $exe
            if ($LASTEXITCODE) {
                throw "Basic complex library execution failed: $LASTEXITCODE"
            }
        }
    }
    foreach ($unit in 'provider', 'client') {
        if ((Get-FileHash "$PSScriptRoot/multiply-env-$unit`1.c1o").Hash -ne (Get-FileHash "$PSScriptRoot/multiply-env-$unit`2.c1o").Hash) {
            throw 'Basic complex library object bytes differ'
        }
    }
    if ((Get-FileHash "$PSScriptRoot/multiply-env-1-1.exe").Hash -ne (Get-FileHash "$PSScriptRoot/multiply-env-2-2.exe").Hash) {
        throw 'Basic complex library executable bytes differ'
    }
    'PASS three multiply-env-unit complex executions, including mixed compiler generations, two equal object pairs and equal executable pair'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}

