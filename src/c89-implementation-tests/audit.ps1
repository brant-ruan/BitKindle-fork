param([Parameter(Mandatory = $true)][string]$ScratchDirectory, [Parameter(Mandatory = $true)][string]$VerificationLog)
$ErrorActionPreference = 'Stop'
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
$entries = Get-Content (Join-Path $PSScriptRoot 'SOURCES.json') -Raw | ConvertFrom-Json
foreach ($entry in $entries) {
    if ($entry.path.StartsWith('templates/')) {
        if ([IO.File]::ReadAllText((Join-Path $ScratchDirectory $entry.path.Substring(10))) -cne [IO.File]::ReadAllText((Join-Path $PSScriptRoot $entry.path))) {
            throw 'Materialized source changed'
        }
    }
}
$outer = Get-Content $VerificationLog -Raw
if (!$outer.TrimEnd().EndsWith('PASS all six implementation suites')) {
    throw 'Missing final runner gate'
}
$pairs = 0
$executions = 0
function Pair([string]$left, [string]$right) {
    $a = [IO.File]::ReadAllBytes($left)
    $b = [IO.File]::ReadAllBytes($right)
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw 'Empty or unequal artifact'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Artifact bytes differ'
        }
    }
    $script:pairs++
}
foreach ($name in 'characters', 'layout', 'pointers', 'rounding', 'math', 'signals') {
    if ([regex]::Matches($outer, '(?m)^PASS implementation suite ' + $name + '\r?$').Count -ne 1) {
        throw 'Missing suite gate'
    }
    $dir = Join-Path $ScratchDirectory $name
    if ($name -eq 'signals') {
        $log = Get-Content (Join-Path $dir 'TESTS.txt') -Raw
        if (!$log.TrimEnd().EndsWith('PASS 42 signal executions and 42 nonempty equal artifact pairs')) {
            throw 'Missing signal completion'
        }
        $cases = @()
        foreach ($sig in 2, 4, 8, 11, 15, 22) {
            foreach ($mode in 0, 1, 2) {
                $cases += @{name = "signal-$sig-$mode"
                    code = $(if ($mode) {
                            0
                        }
                        elseif ($sig -eq 22) {
                            3
                        }
                        else {
                            128 + $sig
                        })
                }
            }
        }
        foreach ($mode in 3, 4, 5) {
            $cases += @{name = "abort-$mode"
                code = 3
            }
        }
        foreach ($case in $cases) {
            foreach ($g in 1, 2) {
                if ([regex]::Matches($log, '(?m)^' + [regex]::Escape("PASS generation $g $($case.name) exit $($case.code)") + '\r?$').Count -ne 1) {
                    throw 'Missing signal execution'
                }
                $executions++
            }
            foreach ($ext in 'c1o', 'exe') {
                Pair (Join-Path $dir "build/$($case.name)-1.$ext") (Join-Path $dir "build/$($case.name)-2.$ext")
            }
        }
    }
    else {
        $logs = @('TESTS.txt')
        if ($name -ne 'math') {
            $logs += 'TESTS-OBJECTS.txt'
        }
        foreach ($file in $logs) {
            $log = Get-Content (Join-Path $dir $file) -Raw
            foreach ($g in 1, 2) {
                if ([regex]::Matches($log, "(?m)^PASS generation $g ").Count -ne 1) {
                    throw 'Missing probe execution'
                }
                $executions++
            }
        }
        if ($name -ne 'math') {
            Pair (Join-Path $dir 'probe1.exe') (Join-Path $dir 'probe2.exe')
        }
        foreach ($ext in 'c1o', 'exe') {
            Pair (Join-Path $dir "object-probe1.$ext") (Join-Path $dir "object-probe2.$ext")
        }
    }
}
if ($executions -ne 60 -or $pairs -ne 56) {
    throw 'Unexpected audit totals'
}
'PASS six suites, materialized source text, 60 executions and 56 nonempty equal artifact pairs'
'This audit does not replace seed provenance or complete C89 conformance review.'
