param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$VerificationLog)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
if (!$VerificationLog) {
    $VerificationLog = Join-Path $BuildDirectory 'NUMERIC.txt'
}
$lines = Get-Content -LiteralPath $VerificationLog
foreach ($mode in 'c89', 'c99') {
    foreach ($g in 1, 2) {
        $gate = "PASS generation $g mode $mode numeric conversion and scanning"
        if (@($lines | Where-Object { $_ -ceq $gate }).Count -ne 1) {
            throw "Missing or repeated execution gate: $gate"
        }
    }
}
$dir = Join-Path $BuildDirectory 'namespace/numeric'
$pairs = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($pattern in "paired-$mode-{0}.c1o", "paired-$mode-{0}.exe", "paired-runtime-$mode-{0}.c1o") {
        $a = [IO.File]::ReadAllBytes((Join-Path $dir ($pattern -f 1)))
        $b = [IO.File]::ReadAllBytes((Join-Path $dir ($pattern -f 2)))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw "Empty/unequal artifacts: $pattern"
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Unequal bytes: $pattern"
            }
        }
        $pairs++
    }
}
if ($pairs -ne 6) {
    throw 'Expected six pairs'
}
'PASS numeric four execution gates and six nonempty equal artifact pairs'
