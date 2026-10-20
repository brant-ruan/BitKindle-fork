param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$SourceRoot = $PSScriptRoot
$SnapshotRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$CompilerDirectory = Join-Path $BuildDirectory 'host'
$OutputDirectory = Join-Path $BuildDirectory 'namespace'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$ErrorActionPreference = 'Stop'
. (Join-Path $SnapshotRoot 'tools/compile.ps1')
$savedOutput = $env:C1_OUTPUT
$savedRuntime = $env:C1_RUNTIME
$savedStandard = $env:C1_STANDARD
try {
    $env:C1_OUTPUT = 'object'
    $env:C1_STANDARD = $null
    foreach ($g in 1, 2) {
        $dir = Join-Path $OutputDirectory "common$g"
        New-Item -ItemType Directory -Force $dir | Out-Null
        $compiler = Join-Path $CompilerDirectory "c1-stage$g.exe"
        $runtimeSource = Join-Path $dir 'runtime.c'
        $appSource = Join-Path $dir 'app.c'
        [IO.File]::WriteAllText($runtimeSource, 'int extension_state;int abs(int n){return n+extension_state;}', [Text.Encoding]::ASCII)
        [IO.File]::WriteAllText($appSource, 'int extension_state=99;int abs(int);int main(void){return abs(42)!=42;}', [Text.Encoding]::ASCII)
        $env:C1_RUNTIME = $null
        Compile $compiler $appSource (Join-Path $dir 'app.c1o')
        foreach ($kind in 'normal', 'isolated') {
            $env:C1_RUNTIME = if ($kind -eq 'isolated') {
                'c89'
            }
            else {
                $null
            }
            $obj = Join-Path $dir "$kind.c1o"
            Compile $compiler $runtimeSource $obj
            $exe = Join-Path $dir "$kind.exe"
            $line = '"' + $exe + '" main "' + $dir + '/app.c1o" "' + $obj + '"'
            $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList $line -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Common isolation link timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw 'Common isolation link failure'
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Common isolation execution timeout'
            }
            $p.WaitForExit()
            $expected = if ($kind -eq 'isolated') {
                0
            }
            else {
                1
            }
            if ($p.ExitCode -ne $expected) {
                throw "Unexpected $kind control result $($p.ExitCode)"
            }
            "PASS generation $g $kind common isolation control exit $expected"
        }
    }
    foreach ($name in 'app.c1o', 'normal.c1o', 'isolated.c1o', 'normal.exe', 'isolated.exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "common1/$name"))
        $b = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "common2/$name"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw "Length differs $name"
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Bytes differ $name"
            }
        }
        "PASS identical $name"
    }
}
finally {
    $env:C1_OUTPUT = $savedOutput
    $env:C1_RUNTIME = $savedRuntime
    $env:C1_STANDARD = $savedStandard
}
