param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$log = Get-Content (Join-Path $BuildDirectory 'LOCALE.txt') -Raw
if (!$log.TrimEnd().EndsWith('PASS C89 locale namespace and C99 members, four clients and six artifact pairs')) {
    throw 'Missing locale final gate'
}
if ([regex]::Matches($log, '(?m)^PASS generation [12] c(?:89|99) locale members and values').Count -ne 4) {
    throw 'Locale execution count mismatch'
}
foreach ($mode in 'c89', 'c99') {
    foreach ($name in "runtime-$mode.c1o", "$mode.c1o", "$mode.exe") {
        $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "namespace/locale1/$name"))
        $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "namespace/locale2/$name"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Locale artifact length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw 'Locale artifact mismatch'
            }
        }
    }
}
'PASS locale four execution gates and six nonempty artifact pairs'