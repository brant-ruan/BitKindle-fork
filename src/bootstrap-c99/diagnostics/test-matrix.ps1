param([Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$ObjectMode)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ErrorActionPreference = 'Stop'
$values = @(
    @('0', '0', $false), @('2147483648', '80000000', $false),
    @('4294967296', '100000000', $false), @('9223372036854775807', '7fffffffffffffff', $false),
    @('9223372036854775808', '8000000000000000', $true), @('18446744073709551615', 'ffffffffffffffff', $true)
)
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_RUNTIME', 'C1_INCLUDE') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = $null
    foreach ($mode in 'c89', 'c99') {
        $env:C1_STANDARD = $mode
        $text = [Text.StringBuilder]::new()
        $count = 0
        foreach ($row in $values) {
            foreach ($suffix in '', 'L', 'l', 'U', 'UL', 'LU', 'LL', 'ULL') {
                $unsigned = $suffix.Contains('U')
                if ($row[2] -and !$unsigned -and ($mode -eq 'c99' -or $suffix -eq 'LL')) {
                    continue
                }
                $literal = $row[0] + $suffix
                $signedComparison = if ($unsigned -or $row[2]) {
                    0
                }
                else {
                    1
                }
                [void]$text.AppendLine("#if ($literal != 0x" + $row[1] + "UL) || ((-1 < $literal) != $signedComparison)")
                [void]$text.AppendLine("#error wrong value or signedness case $count")
                [void]$text.AppendLine('#endif')
                $count++
            }
        }
        [void]$text.AppendLine('int main(void){return 0;}')
        foreach ($g in 1, 2) {
            $dir = Join-Path $BuildDirectory "matrix/$g"
            New-Item -ItemType Directory -Force $dir | Out-Null
            $source = Join-Path $dir "$mode.c"
            $exe = Join-Path $dir "$mode.exe"
            $err = Join-Path $dir "$mode.stderr"
            [IO.File]::WriteAllText($source, $text.ToString(), [Text.Encoding]::ASCII)
            $p = Start-Process -FilePath (Join-Path $BuildDirectory "host/c1-stage$g.exe") -WindowStyle Hidden -PassThru -RedirectStandardInput $source -RedirectStandardOutput $exe -RedirectStandardError $err
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Matrix compile timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Matrix compile failed: $mode"
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Matrix execution timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw 'Matrix execution failed'
            }
            "PASS generation $g $mode $count preprocessing value/signedness combinations"
        }
        $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "matrix/1/$mode.exe"))
        $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "matrix/2/$mode.exe"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Matrix artifact length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw 'Matrix artifact mismatch'
            }
        }
    }
    'PASS both mode matrices and equal compiler-generation artifacts'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
