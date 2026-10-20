param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build'),
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null

function Invoke-A0([string]$Executable, [string]$Source, [string]$Output) {
    $process = Start-Process -FilePath $Executable -WindowStyle Hidden -PassThru `
        -RedirectStandardInput $Source -RedirectStandardOutput $Output
    [void]$process.Handle
    if (-not $process.WaitForExit(10000)) {
        $process.Kill()
        throw "Assembler timed out: $Executable"
    }
    # Refresh the cached process state before inspecting ExitCode.
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        throw "Assembler failed: $($process.ExitCode)" 
    }
}

$seed = Join-Path $BuildDirectory 'a0-seed.exe'
$stage1 = Join-Path $BuildDirectory 'a0-stage1.exe'
$stage2 = Join-Path $BuildDirectory 'a0-stage2.exe'
$source = Join-Path $PSScriptRoot 'a0.a0'

# The one external binary-producing operation. It only decodes literal bytes.
& (Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe') -NoProfile -NonInteractive -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'decode_seed.ps1') -Source (Join-Path $PSScriptRoot 'seed.hex') -Output $seed
if ($LASTEXITCODE -ne 0) {
    throw 'Seed decoding failed' 
}

# Redirection copies the assembler's output bytes; it does not transform them.
Invoke-A0 $seed $source $stage1
Invoke-A0 $stage1 $source $stage2
$hashes = @($seed, $stage1, $stage2 | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash })
if (($hashes | Select-Object -Unique).Count -ne 1) {
    throw 'Bootstrap hash mismatch' 
}
$first = [IO.File]::ReadAllBytes($stage1)
$second = [IO.File]::ReadAllBytes($stage2)
if ($first.Length -ne $second.Length) {
    throw 'Bootstrap length mismatch' 
}
for ($i = 0; $i -lt $first.Length; $i++) {
    if ($first[$i] -ne $second[$i]) {
        throw "Bootstrap byte mismatch at $i" 
    }
}
if (-not $SkipTests) {
    & (Join-Path $PSScriptRoot 'tests/test-a0.ps1') -Assembler $stage2 -ScratchDirectory (Join-Path $BuildDirectory 'tests')
    if (-not $?) {
        throw 'Tests failed' 
    }
}
"PASS: seed = stage1 = stage2 ($($first.Length) bytes)"
"SHA256: $($hashes[0])"

$a1Source = Join-Path $PSScriptRoot 'a1.a0'
$a1stage0 = Join-Path $BuildDirectory 'a1-stage0.exe'
$a1stage1 = Join-Path $BuildDirectory 'a1-stage1.exe'
$a1stage2 = Join-Path $BuildDirectory 'a1-stage2.exe'
Invoke-A0 $stage2 $a1Source $a1stage0
Invoke-A0 $a1stage0 $a1Source $a1stage1
Invoke-A0 $a1stage1 $a1Source $a1stage2
$a1Hashes = @($a1stage0, $a1stage1, $a1stage2 | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash })
if (($a1Hashes | Select-Object -Unique).Count -ne 1) {
    throw 'A1 bootstrap mismatch' 
}
if (-not $SkipTests) {
    & (Join-Path $PSScriptRoot 'tests/test-a1.ps1') -Assembler $a1stage2 -ScratchDirectory (Join-Path $BuildDirectory 'tests-a1')
    if (-not $?) {
        throw 'A1 tests failed' 
    }
}
"PASS: A1 stage0 = stage1 = stage2"
"SHA256: $($a1Hashes[0])"

$c0Source = Join-Path $PSScriptRoot 'c0.a1'
$c0 = Join-Path $BuildDirectory 'c0.exe'
Invoke-A0 $a1stage2 $c0Source $c0
if (-not $SkipTests) {
    & (Join-Path $PSScriptRoot 'tests/test-c0.ps1') -Compiler $c0 -Assembler $a1stage2 -ScratchDirectory (Join-Path $BuildDirectory 'tests-c0')
    if (-not $?) {
        throw 'C0 tests failed' 
    }
}
"C0 SHA256: $((Get-FileHash -LiteralPath $c0 -Algorithm SHA256).Hash)"

if (-not $SkipTests) {
    $lexerSource = Join-Path $PSScriptRoot 'c1/lexer.c'
    $lexerAssembly = Join-Path $BuildDirectory 'c1-lexer.a1'
    $lexerAssemblyRepeat = Join-Path $BuildDirectory 'c1-lexer-repeat.a1'
    $lexer = Join-Path $BuildDirectory 'c1-lexer.exe'
    Invoke-A0 $c0 $lexerSource $lexerAssembly
    Invoke-A0 $c0 $lexerSource $lexerAssemblyRepeat
    if ((Get-FileHash $lexerAssembly).Hash -ne (Get-FileHash $lexerAssemblyRepeat).Hash) {
        throw 'C0 nondeterministic output' 
    }
    Invoke-A0 $a1stage2 $lexerAssembly $lexer
    & (Join-Path $PSScriptRoot 'tests/test-c1-lexer.ps1') -Lexer $lexer -ScratchDirectory (Join-Path $BuildDirectory 'tests-lexer')
    if (-not $?) {
        throw 'C1 lexer tests failed' 
    }
    "PASS: C-written C1 lexer built by C0 and A1; repeated C0 output identical"
    "C1 lexer SHA256: $((Get-FileHash -LiteralPath $lexer -Algorithm SHA256).Hash)"
}

& (Join-Path $PSScriptRoot 'build-c1.ps1') -BuildDirectory $BuildDirectory -SkipTests:$SkipTests
if (-not $?) {
    throw 'C1 self-compilation failed' 
}
