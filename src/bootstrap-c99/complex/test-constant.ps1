param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'")).Replace("'^C1 error: '", "'(?m)^C1 error: '")
. (Join-Path $PSScriptRoot 'constant-cases.ps1')
$commands = @()
foreach ($n in $positive.Keys) {
    $commands += "Check '$n' '$($positive[$n])' 0"
}
foreach ($n in $negative.Keys) {
    $commands += "Check 'reject-$n' '$($negative[$n])' 0 `$true"
}
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($g in 1, 2) {
        & ([scriptblock]::Create($prefix + ($commands -join "`n"))) -Compiler "$BuildDirectory/host/c1-stage$g.exe" -Assembler "$BuildDirectory/link-stage$g.exe" -CompilerKind C1 -ScratchDirectory "$PSScriptRoot/constant$g"
        if (!$?) {
            throw 'Complex constant failed'
        }
    }
    foreach ($n in $positive.Keys) {
        if ((Get-FileHash "$PSScriptRoot/constant1/$n.exe").Hash -ne (Get-FileHash "$PSScriptRoot/constant2/$n.exe").Hash) {
            throw 'Complex generation mismatch'
        }
    }
    "PASS $($positive.Count*2) complex constant executions, $($negative.Count*2) constraint rejections and $($positive.Count) equal pairs"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
