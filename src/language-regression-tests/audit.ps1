param([Parameter(Mandatory = $true)][string]$ScratchDirectory, [Parameter(Mandatory = $true)][string]$VerificationLog)
$ErrorActionPreference = 'Stop'
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
$entries = Get-Content (Join-Path $PSScriptRoot 'SOURCES.json') -Raw | ConvertFrom-Json
foreach ($entry in $entries) {
    if ($entry.path.StartsWith('templates/') -and [IO.File]::ReadAllText((Join-Path $ScratchDirectory $entry.path.Substring(10))) -cne [IO.File]::ReadAllText((Join-Path $PSScriptRoot $entry.path))) {
        throw 'Materialized source changed'
    }
}
$outer = Get-Content $VerificationLog -Raw
if (!$outer.TrimEnd().EndsWith('PASS all three review suites')) {
    throw 'Missing final gate'
}
function Gate([string]$log, [string]$line, [int]$count = 1) {
    if ([regex]::Matches($log, '(?m)^' + [regex]::Escape($line) + '\r?$').Count -ne $count) {
        throw "Missing/duplicate gate: $line"
    }
}
$pairs = 0
function Pair([string]$aPath, [string]$bPath) {
    $a = [IO.File]::ReadAllBytes($aPath)
    $b = [IO.File]::ReadAllBytes($bPath)
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw 'Empty or unequal pair'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Bytes differ'
        }
    }
    $script:pairs++
}
foreach ($name in 'preprocessor', 'rescan', 'floating') {
    Gate $outer "PASS review suite $name"
    $dir = Join-Path $ScratchDirectory $name
    $log = Get-Content (Join-Path $dir 'TESTS.txt') -Raw
    if ($name -eq 'preprocessor') {
        Gate $log 'PASS 25 preprocessing expression cases' 4
        Gate $log 'PASS 25 preprocessing expression cases in each mode and generation'
        $cases = Get-Content (Join-Path $dir 'CASES.json') -Raw | ConvertFrom-Json
        if ($cases.Count -ne 25 -or @($cases | Where-Object reject).Count -ne 5) {
            throw 'Unexpected case inventory'
        }
        foreach ($case in $cases) {
            Gate $log "PASS C1 $($case.name)" 4
            foreach ($mode in 'c89', 'c99') {
                if ($case.reject) {
                    foreach ($g in 1, 2) {
                        $exe = Join-Path $dir "$mode-$g/$($case.name).exe"
                        $err = Join-Path $dir "$mode-$g/$($case.name).err"
                        if ((Get-Item $exe).Length -ne 0 -or [IO.File]::ReadAllText($err) -notmatch '(?m)^C1 error: ') {
                            throw 'Missing rejection evidence'
                        }
                    }
                }
                else {
                    Pair (Join-Path $dir "$mode-1/$($case.name).exe") (Join-Path $dir "$mode-2/$($case.name).exe")
                }
            }
        }
    }
    else {
        $label = if ($name -eq 'rescan') {
            'macro rescan probe'
        }
        else {
            'pragma probe'
        }
        foreach ($g in 1, 2) {
            Gate $log "PASS $label generation $g"
        }
        foreach ($ext in 'c1o', 'exe') {
            Pair (Join-Path $dir "probe1.$ext") (Join-Path $dir "probe2.$ext")
        }
    }
}
if ($pairs -ne 44) {
    throw 'Unexpected pair count'
}
'PASS three review suites, 100 preprocessing checks, four probe executions and 44 equal artifact pairs'
