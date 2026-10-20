param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$log = Get-Content (Join-Path $BuildDirectory 'CLOCK.txt')
foreach ($g in 1, 2) {
    if (@($log | Where-Object { $_ -ceq "PASS fixed generation $g clock test exit 0" }).Count -ne 1) {
        throw 'Missing/repeated clock gate'
    }
}
foreach ($ext in 'c1o', 'exe') {
    $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "namespace/clock/fixed1.$ext"))
    $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "namespace/clock/fixed2.$ext"))
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw 'Empty/unequal clock artifacts'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Clock artifact mismatch'
        }
    }
}
'PASS clock fallback and cache: 15 scenarios per generation and two equal artifact pairs'