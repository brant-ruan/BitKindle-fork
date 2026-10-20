param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [switch]$SkipTests
)
. (Join-Path $PSScriptRoot 'ps51-compat.ps1')

$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
function Compile([string]$Tool, [string]$Source, [string]$Output) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    if ([IO.Path]::GetFileName($Tool) -match '^c1-(stage[0-9]+|bridge|capacity)\.exe$') {
        # Only buffer the original bytes flowing through standard streams.
        # C0/A1 need regular-file EOF semantics and retain the path below.
        $info = [Diagnostics.ProcessStartInfo]::new($Tool)
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $info.RedirectStandardInput = $true
        $info.RedirectStandardOutput = $true
        $process = [Diagnostics.Process]::new()
        $process.StartInfo = $info
        $sourceStream = $null
        $outputStream = $null
        $started = $false
        try {
            $sourceStream = [IO.File]::OpenRead($Source)
            $outputStream = [IO.File]::Create($Output)
            [void]$process.Start()
            $started = $true
            $outTask = $process.StandardOutput.BaseStream.CopyToAsync($outputStream)
            $inTask = $sourceStream.CopyToAsync($process.StandardInput.BaseStream)
            if (-not $inTask.Wait(120000)) {
                throw "Input timeout: $Tool"
            }
            $process.StandardInput.Close()
            if (-not $process.WaitForExit(120000)) {
                throw "Compile timeout: $Tool"
            }
            if (-not $outTask.Wait(120000)) {
                throw "Output timeout: $Tool"
            }
            if ($process.ExitCode -ne 0) {
                throw "Compile failed ($($process.ExitCode)): $Tool"
            }
        }
        finally {
            if ($started -and -not $process.HasExited) {
                $process.Kill()
            }
            if ($sourceStream) {
                $sourceStream.Dispose()
            }
            if ($outputStream) {
                $outputStream.Dispose()
            }
            $process.Dispose()
        }
    }
    else {
        $p = Start-Process -FilePath $Tool -WindowStyle Hidden -PassThru -RedirectStandardInput $Source -RedirectStandardOutput $Output
        [void]$p.Handle
        if (-not $p.WaitForExit(120000)) {
            $p.Kill()
            throw "Compile timeout: $Tool" 
        }
        $p.WaitForExit()
        if ($p.ExitCode -ne 0) {
            throw "Compile failed ($($p.ExitCode)): $Tool" 
        }
    }
    "Built $([IO.Path]::GetFileName($Output)) in $([C1Math]::Round($watch.Elapsed.TotalSeconds,2)) s"
}
$source = Join-Path $PSScriptRoot 'c1/compiler.c'
$c0 = Join-Path $BuildDirectory 'c0.exe'
$a1 = Join-Path $BuildDirectory 'a1-stage2.exe'
$assembly = Join-Path $BuildDirectory 'c1-bridge.a1'
$bridge = Join-Path $BuildDirectory 'c1-bridge.exe'
$capacity = Join-Path $BuildDirectory 'c1-capacity.exe'
$stage0 = Join-Path $BuildDirectory 'c1-stage0.exe'
$stage1 = Join-Path $BuildDirectory 'c1-stage1.exe'
$stage2 = Join-Path $BuildDirectory 'c1-stage2.exe'
Compile $c0 (Join-Path $PSScriptRoot 'c1/bootstrap.c') $assembly
Compile $a1 $assembly $bridge
Compile $bridge (Join-Path $PSScriptRoot 'c1/bootstrap-capacity.c') $capacity
if (-not $SkipTests) {
    & (Join-Path $PSScriptRoot 'tests/test-c0.ps1') -Compiler $capacity -Assembler $a1 -CompilerKind C1 -CapacityBridgeOnly -ScratchDirectory (Join-Path $BuildDirectory 'tests-capacity-bridge')
    if (-not $?) {
        throw 'Capacity bridge tests failed' 
    }
}
Compile $capacity $source $stage0
Compile $stage0 $source $stage1
Compile $stage1 $source $stage2
$left = [IO.File]::ReadAllBytes($stage1)
$right = [IO.File]::ReadAllBytes($stage2)
if ($left.Length -ne $right.Length) {
    throw 'C1 self-compile length mismatch' 
}
for ($i = 0; $i -lt $left.Length; $i = $i + 1) {
    if ($left[$i] -ne $right[$i]) {
        throw "C1 self-compile mismatch at byte $i" 
    }
}
"PASS: C1 stage1 = stage2, byte-for-byte ($($left.Length) bytes)"
"C1 self-compile SHA256: $((Get-FileHash -LiteralPath $stage2 -Algorithm SHA256).Hash)"
if ($SkipTests) {
    'BUILT base C1; regression tests skipped'
    return 
}
& (Join-Path $PSScriptRoot 'tools/build-pi-table.ps1') -Stage1 $stage1 -Stage2 $stage2 -BuildDirectory $BuildDirectory
if (-not $?) {
    throw 'Pi table rebuild failed' 
}
if (-not $SkipTests) {
    & (Join-Path $PSScriptRoot 'tests/test-c0.ps1') -Compiler $stage1 -Assembler $a1 -CompilerKind C1 -ScratchDirectory (Join-Path $BuildDirectory 'tests-c1-stage1')
    if (-not $?) {
        throw 'C1 stage1 tests failed' 
    }
}
if (-not $SkipTests) {
    & (Join-Path $PSScriptRoot 'tests/test-c0.ps1') -Compiler $stage2 -Assembler $a1 -CompilerKind C1 -ScratchDirectory (Join-Path $BuildDirectory 'tests-c1-stage2')
    if (-not $?) {
        throw 'C1 stage2 tests failed' 
    }
}
$lexer = Join-Path $BuildDirectory 'c1-lexer-self.exe'
Compile $stage2 (Join-Path $PSScriptRoot 'c1/lexer.c') $lexer
& (Join-Path $PSScriptRoot 'tests/test-c1-lexer.ps1') -Lexer $lexer -ScratchDirectory (Join-Path $BuildDirectory 'tests-lexer-self') -ReferenceTokens (Join-Path $BuildDirectory 'tests-lexer/own-source.tokens')
if (-not $?) {
    throw 'Self-compiled C1 lexer tests failed' 
}
"PASS: C1 self-compilation and both generations' functional tests"
