#requires -Version 5.1
param(
    [Parameter(Mandatory = $true)][ValidateSet('Seed', 'A0', 'A1', 'C0', 'Lexer', 'C1', 'Toolchain')][string]$Stage
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$snapshot = Join-Path $root 'src/bootstrap-c99'
$build = Join-Path $root 'work/walkthrough'
$base = Join-Path $build 'base'
$src = Join-Path $snapshot 'base'
$saved = @{}
foreach ($name in 'C1_INCLUDE', 'C1_OUTPUT', 'C1_STANDARD', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
Push-Location $root
try {
    $env:C1_INCLUDE = $null
    $env:C1_OUTPUT = $null
    $env:C1_STANDARD = $null
    $env:C1_RUNTIME = $null
    . "$snapshot/tools/compile.ps1"
    function Require([string]$path) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Missing prerequisite: $path; run the previous stage first"
        }
    }
    function Same([string[]]$paths) {
        $hashes = @($paths | ForEach-Object { Require $_; (Get-FileHash -LiteralPath $_).Hash })
        if (($hashes | Select-Object -Unique).Count -ne 1) {
            throw 'Stage identity mismatch'
        }
        "PASS identical SHA256: $($hashes[0])"
    }
    $marker = Join-Path $build ($Stage + '.success')
    $log = Join-Path $build ($Stage + '.log')
    if (Test-Path -LiteralPath $log) {
        throw 'Stage already attempted. Inspect its log; use a fresh extraction to restart without overwriting evidence.'
    }
    New-Item -ItemType Directory -Force $base | Out-Null
    $run = {
        switch ($Stage) {
            Seed {
                $seedShell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
                & $seedShell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$src/decode_seed.ps1" -Source "$src/seed.hex" -Output "$base/a0-seed.exe"
                if ($LASTEXITCODE) {
                    throw 'Seed decoding failed'
                }
                if ((Get-FileHash "$base/a0-seed.exe").Hash -ne 'BA0F84742B07A4A5340D0641B676EE18269943191139B570667D9984A98465E5') {
                    throw 'Seed hash mismatch'
                }
            }
            A0 {
                Require "$base/a0-seed.exe"
                Compile "$base/a0-seed.exe" "$src/a0.a0" "$base/a0-stage1.exe"
                Compile "$base/a0-stage1.exe" "$src/a0.a0" "$base/a0-stage2.exe"
                Same @("$base/a0-seed.exe", "$base/a0-stage1.exe", "$base/a0-stage2.exe")
                & "$src/tests/test-a0.ps1" -Assembler "$base/a0-stage2.exe" -ScratchDirectory "$base/tests"
                if (!$?) {
                    throw 'A0 tests failed'
                }
            }
            A1 {
                Require "$base/a0-stage2.exe"
                Compile "$base/a0-stage2.exe" "$src/a1.a0" "$base/a1-stage0.exe"
                Compile "$base/a1-stage0.exe" "$src/a1.a0" "$base/a1-stage1.exe"
                Compile "$base/a1-stage1.exe" "$src/a1.a0" "$base/a1-stage2.exe"
                Same @("$base/a1-stage0.exe", "$base/a1-stage1.exe", "$base/a1-stage2.exe")
                & "$src/tests/test-a1.ps1" -Assembler "$base/a1-stage2.exe" -ScratchDirectory "$base/tests-a1"
                if (!$?) {
                    throw 'A1 tests failed'
                }
            }
            C0 {
                Require "$base/a1-stage2.exe"
                Compile "$base/a1-stage2.exe" "$src/c0.a1" "$base/c0.exe"
                & "$src/tests/test-c0.ps1" -Compiler "$base/c0.exe" -Assembler "$base/a1-stage2.exe" -ScratchDirectory "$base/tests-c0"
                if (!$?) {
                    throw 'C0 tests failed'
                }
            }
            Lexer {
                Require "$base/c0.exe"
                Compile "$base/c0.exe" "$src/c1/lexer.c" "$base/c1-lexer.a1"
                Compile "$base/c0.exe" "$src/c1/lexer.c" "$base/c1-lexer-repeat.a1"
                Same @("$base/c1-lexer.a1", "$base/c1-lexer-repeat.a1")
                Compile "$base/a1-stage2.exe" "$base/c1-lexer.a1" "$base/c1-lexer.exe"
                & "$src/tests/test-c1-lexer.ps1" -Lexer "$base/c1-lexer.exe" -ScratchDirectory "$base/tests-lexer"
                if (!$?) {
                    throw 'Lexer tests failed'
                }
            }
            C1 {
                Require "$base/tests-lexer/own-source.tokens"
                & "$src/build-c1.ps1" -BuildDirectory $base
                if (!$?) {
                    throw 'C1 bootstrap failed'
                }
            }
            Toolchain {
                Require "$base/c1-stage2.exe"
                & "$snapshot/build-toolchain.ps1" -BaseBuild $base -BuildDirectory $build
                if (!$?) {
                    throw 'Final toolchain build failed'
                }
                if ((Get-FileHash "$build/host/c1-stage2.exe").Hash -ne 'DB92844256433C7020D0C399E8F32D9CB9BE4954CC4063EAC0DFEAD0A4570ADD') {
                    throw 'Final compiler baseline mismatch'
                }
            }
        }
        "PASS walkthrough stage $Stage"
    }
    Write-Host "Running $Stage; log: work/walkthrough/$Stage.log"
    & $run *> $log
    if (!$?) {
        throw "Stage failed; see $log"
    }
    [IO.File]::WriteAllText($marker, 'PASS ' + [DateTime]::UtcNow.ToString('o'))
    Get-Content -LiteralPath $log -Tail 5
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
    Pop-Location
}
