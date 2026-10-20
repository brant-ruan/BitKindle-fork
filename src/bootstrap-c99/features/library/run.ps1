param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$sourceRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. (Join-Path $sourceRoot 'tools/compile.ps1')
$savedInclude = $env:C1_INCLUDE
$savedOutput = $env:C1_OUTPUT
$kinds = @('inttypes', 'wctype', 'wchar-string', 'restartable', 'wide-integer', 'wide-float', 'wide-time', 'wide-io', 'wide-output', 'wide-input', 'c89-addresses')
try {
    $env:C1_INCLUDE = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $sourceRoot $_ }) -join ';'
    $apis = Get-Content (Join-Path $sourceRoot 'runtime/imports.json') -Raw | ConvertFrom-Json
    $options = ($apis | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        $compiler = Join-Path $BuildDirectory "host/c1-stage$g.exe"
        $linker = Join-Path $BuildDirectory "link-stage$g.exe"
        $runtime = Join-Path $BuildDirectory "host/runtime$g.c1o"
        $outdir = Join-Path $BuildDirectory "features/library$g"
        New-Item -ItemType Directory -Force $outdir | Out-Null
        foreach ($kind in $kinds) {
            $source = if ($kind -eq 'c89-addresses') {
                Join-Path $PSScriptRoot 'c89-addresses.c'
            }
            else {
                Join-Path $PSScriptRoot "$kind/tests.c"
            }
            $obj = Join-Path $outdir "$kind.c1o"
            $exe = Join-Path $outdir "$kind.exe"
            $wrapper = Join-Path $outdir "$kind-input.c"
            [IO.File]::WriteAllText($wrapper, ("#include <stdio.h>" + [char]10 + '#include "' + $source.Replace('\', '/') + '"'))
            $env:C1_OUTPUT = 'object'
            Compile $compiler $wrapper $obj
            $env:C1_OUTPUT = $null
            $args = '"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options
            $err = Join-Path $outdir "$kind.link.stderr"
            $p = Start-Process -FilePath $linker -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardError $err
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Library link timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Library link failed: $kind"
            }
            $inputFile = Join-Path $outdir "$kind.stdin"
            [IO.File]::WriteAllText($inputFile, $(if ($kind -eq 'wide-input') {
                        '17 25'
                    }
                    else {
                        ''
                    }))
            $stdout = Join-Path $outdir "$kind.stdout"
            $stderr = Join-Path $outdir "$kind.stderr"
            $p = Start-Process -FilePath $exe -WorkingDirectory $outdir -WindowStyle Hidden -PassThru -RedirectStandardInput $inputFile -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            [void]$p.Handle
            if (!$p.WaitForExit(60000)) {
                $p.Kill()
                throw "Library execution timeout: $kind"
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Library $kind generation $g returned $($p.ExitCode)"
            }
            $expected = if ($kind -eq 'wide-output') {
                "W:42`r`n"
            }
            else {
                ''
            }
            if ([IO.File]::ReadAllText($stdout) -cne $expected -or [IO.File]::ReadAllText($stderr) -cne '') {
                throw "Library stream output mismatch: $kind"
            }
            "PASS generation $g cumulative library $kind"
        }
    }
    foreach ($kind in $kinds) {
        foreach ($ext in '.c1o', '.exe') {
            $name = $kind + $ext
            $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "features/library1/$name"))
            $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "features/library2/$name"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw "Library length mismatch: $name"
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw "Library bytes differ: $name"
                }
            }
            "PASS identical library $name"
        }
    }
    'PASS both generations cumulative library suites'
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $env:C1_OUTPUT = $savedOutput
}
