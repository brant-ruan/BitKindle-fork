param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/ucn'
$log = Get-Content (Join-Path $BuildDirectory 'UCN.txt') -Raw
$cases = Get-Content (Join-Path $PSScriptRoot 'ucn/cases.json') -Raw | ConvertFrom-Json
if ($cases.Count -ne 71 -or @($cases | Where-Object { $_.reject }).Count -ne 23) {
    throw 'Unexpected UCN case inventory'
}
function Same([string]$a, [string]$b) {
    $left = [IO.File]::ReadAllBytes($a)
    $right = [IO.File]::ReadAllBytes($b)
    if (!$left.Length -or $left.Length -ne $right.Length) {
        throw 'Empty or unequal UCN artifacts'
    }
    for ($i = 0; $i -lt $left.Length; $i++) {
        if ($left[$i] -ne $right[$i]) {
            throw 'Unequal UCN bytes'
        }
    }
}
$pairs = 0
$rejects = 0
foreach ($c in $cases) {
    if ([regex]::Matches($log, '(?m)^PASS C1 ' + [regex]::Escape($c.name) + '\r?$').Count -ne 4) {
        throw "Missing UCN case gate $($c.name)"
    }
    foreach ($mode in 'c89', 'c99') {
        foreach ($g in 1, 2) {
            $path = "$root/$mode-$g/$($c.name)"
            $diagnostic = [IO.File]::ReadAllText("$path.err")
            if ($c.reject) {
                if ((Get-Item "$path.exe").Length -ne 0 -or $diagnostic -notmatch '(?m)^C1 error: ') {
                    throw 'Missing rejected artifact evidence'
                }
                $rejects++
            }
            elseif (!$c.control) {
                if ($mode -eq 'c89' -and $diagnostic -notmatch 'C99 extension in C89 mode: universal character name') {
                    throw 'Missing extension diagnostic'
                }
                if ($mode -eq 'c99' -and $diagnostic.Length) {
                    throw 'Unexpected C99 diagnostic'
                }
            }
            if ($c.name -eq 'header-location-invalid-initial' -and !$diagnostic.Contains('"ucn-location.h":73:')) {
                throw 'Wrong initial-digit diagnostic location'
            }
            if ($c.name -eq 'header-location-truncated' -and !$diagnostic.Contains('"ucn-location.h":97:')) {
                throw 'Wrong malformed UCN diagnostic location'
            }
        }
        if (!$c.reject) {
            Same "$root/$mode-1/$($c.name).exe" "$root/$mode-2/$($c.name).exe"
            $pairs++
        }
    }
}
foreach ($unit in 'caller', 'callee') {
    Same "$root/${unit}1.c1o" "$root/${unit}2.c1o"
    $pairs++
}
Same "$root/cross1.exe" "$root/cross2.exe"
$pairs++
foreach ($g in 1, 2) {
    if (!($log -match "(?m)^PASS UCN cross-unit generation $g\r?$")) {
        throw 'Missing cross-unit execution gate'
    }
}
if (!$log.TrimEnd().EndsWith('PASS all packaged UCN executions and rejections')) {
    throw 'Missing UCN completion gate'
}
if ($pairs -ne 99 -or $rejects -ne 92) {
    throw 'Incorrect UCN audit totals'
}
'PASS UCN: 194 executions, 92 rejections, eight location checks and 99 identical artifact pairs'
