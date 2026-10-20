param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/character-sign'
$log = Get-Content (Join-Path $BuildDirectory 'CHARACTER-SIGN.txt') -Raw
$cases = Get-Content (Join-Path $PSScriptRoot 'character-sign/cases.json') -Raw | ConvertFrom-Json
if ($cases.Count -ne 6) {
    throw 'Character case inventory mismatch'
}
function Same([string]$a, [string]$b) {
    $left = [IO.File]::ReadAllBytes($a)
    $right = [IO.File]::ReadAllBytes($b)
    if (!$left.Length -or $left.Length -ne $right.Length) {
        throw 'Empty or unequal character artifacts'
    }
    for ($i = 0; $i -lt $left.Length; $i++) {
        if ($left[$i] -ne $right[$i]) {
            throw 'Character artifact mismatch'
        }
    }
}
$pairs = 0
foreach ($c in $cases) {
    if ([regex]::Matches($log, '(?m)^PASS C1 ' + [regex]::Escape($c.name) + '\r?$').Count -ne 4) {
        throw 'Missing character case gate'
    }
    foreach ($mode in 'c89', 'c99') {
        Same "$root/fixed-$mode-1/$($c.name).exe" "$root/fixed-$mode-2/$($c.name).exe"
        $pairs++
    }
}
foreach ($mode in 'c89', 'c99') {
    foreach ($g in 1, 2) {
        if (!$log.Contains("PASS raw source $mode generation ${g}: 512 runtime and 128 preprocessing checks")) {
            throw 'Missing raw execution gate'
        }
    }
    foreach ($ext in 'c1o', 'exe') {
        Same "$root/raw/$mode-1.$ext" "$root/raw/$mode-2.$ext"
        $pairs++
    }
}
if ($pairs -ne 16 -or !$log.TrimEnd().EndsWith('PASS complete character sign suites')) {
    throw 'Missing character completion gate'
}
'PASS character sign: 28 executions and 16 identical artifact pairs, including all raw high-byte values'