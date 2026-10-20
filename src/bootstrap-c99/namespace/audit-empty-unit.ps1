param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/empty-unit/fixed'
$log = [IO.File]::ReadAllText((Join-Path $BuildDirectory 'EMPTY-UNIT.txt'))
$negative = @('empty', 'whitespace', 'comment', 'macro', 'skipped', 'expansion')
$positive = @('typedef', 'tag', 'prototype', 'external')
$rejected = 0
$accepted = 0
$pairs = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($name in $negative + $positive) {
        foreach ($g in 1, 2) {
            $path = "$root/$mode-$g/$name"
            if (!$log.Contains("PASS fixed $mode generation $g $name")) {
                throw 'Missing outcome gate'
            }
            $bytes = [IO.File]::ReadAllBytes("$path.c1o")
            $err = [IO.File]::ReadAllText("$path.err")
            if ($name -in $negative) {
                if ($bytes.Length -or $err.Trim() -cne 'C1 error: empty translation unit') {
                    throw 'Invalid empty-unit rejection evidence'
                }
                $rejected++
            }
            else {
                if (!$bytes.Length -or $err.Length) {
                    throw 'Invalid declaration-only acceptance evidence'
                }
                $accepted++
            }
        }
        if ($name -in $positive) {
            $a = [IO.File]::ReadAllBytes("$root/$mode-1/$name.c1o")
            $b = [IO.File]::ReadAllBytes("$root/$mode-2/$name.c1o")
            if ($a.Length -ne $b.Length) {
                throw 'Object length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Object byte mismatch'
                }
            }
            $pairs++
        }
    }
}
if ($rejected -ne 24 -or $accepted -ne 16 -or $pairs -ne 8) {
    throw 'Wrong audit totals'
}
if (!$log.TrimEnd().EndsWith('PASS 24 empty-unit diagnostic rejections; 16 valid declarations accepted; 8 equal object pairs')) {
    throw 'Missing final gate'
}
'PASS empty translation units: 24 rejections, 16 valid declarations, 8 equal object pairs'
