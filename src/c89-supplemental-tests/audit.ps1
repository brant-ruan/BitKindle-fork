param(
    [string]$Sources = $PSScriptRoot,
    [Parameter(Mandatory = $true)][string]$ScratchDirectory,
    [Parameter(Mandatory = $true)][string]$VerificationLog,
    [ValidateSet('headers', 'macros', 'multibyte', 'search', 'facts', 'integers', 'limits')][string[]]$Cases = @('headers', 'macros', 'multibyte', 'search', 'facts', 'integers', 'limits')
)
$ErrorActionPreference = 'Stop'
$Sources = [IO.Path]::GetFullPath($Sources)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
$entries = Get-Content (Join-Path $Sources 'SOURCES.json') -Raw | ConvertFrom-Json
$outer = Get-Content -LiteralPath $VerificationLog -Raw
if (!$outer.TrimEnd().EndsWith('PASS selected supplemental test suites')) {
    throw 'Missing runner final gate'
}
if ([regex]::Matches($outer, '(?m)^PASS supplemental ').Count -ne $Cases.Count) {
    throw 'Runner case count mismatch'
}
$pairs = 0
function Pair([string]$a, [string]$b) {
    $left = [IO.File]::ReadAllBytes($a)
    $right = [IO.File]::ReadAllBytes($b)
    if (!$left.Length -or $left.Length -ne $right.Length) {
        throw "Empty or unequal artifact: $a"
    }
    for ($i = 0; $i -lt $left.Length; $i++) {
        if ($left[$i] -ne $right[$i]) {
            throw "Artifact mismatch: $a"
        }
    }
    $script:pairs++
}
foreach ($name in $Cases) {
    if ([regex]::Matches($outer, '(?m)^' + [regex]::Escape("PASS supplemental $name") + '\r?$').Count -ne 1) {
        throw "Missing runner case: $name"
    }
    $dir = Join-Path $ScratchDirectory $name
    foreach ($entry in $entries | Where-Object { $_.path.StartsWith("templates/$name/") }) {
        if ([IO.File]::ReadAllText((Join-Path $dir ([IO.Path]::GetFileName($entry.path)))) -cne [IO.File]::ReadAllText((Join-Path $Sources $entry.path))) {
            throw "Materialized source mismatch: $($entry.path)"
        }
    }
    $log = Get-Content (Join-Path $dir 'TESTS.txt') -Raw
    if ($name -eq 'headers') {
        if (!$log.TrimEnd().EndsWith('PASS 480 C89 header order executions and 240 nonempty equal executable pairs')) {
            throw 'Incomplete header tests'
        }
        $headers = 'assert ctype errno float limits locale math setjmp signal stdarg stddef stdio stdlib string time'.Split(' ')
        $names = @()
        foreach ($a in $headers) {
            foreach ($b in $headers) {
                if ($a -ne $b) {
                    $names += "pair-$a-$b"
                }
            }
        }
        foreach ($direction in 1, -1) {
            for ($start = 0; $start -lt 15; $start++) {
                $names += "all-$direction-$start"
            }
        }
        foreach ($case in $names) {
            foreach ($g in 1, 2) {
                if ([regex]::Matches($log, '(?m)^' + [regex]::Escape("PASS generation $g headers $case") + '\r?$').Count -ne 1) {
                    throw 'Missing header execution'
                }
            }
            Pair (Join-Path $dir "generation1/$case.exe") (Join-Path $dir "generation2/$case.exe")
        }
    }
    elseif ($name -eq 'macros') {
        if (!$log.TrimEnd().EndsWith('PASS 18 diagnostic cases and 9 nonempty equal executable pairs')) {
            throw 'Incomplete direct macro tests'
        }
        $objects = Get-Content (Join-Path $dir 'TESTS-OBJECTS.txt') -Raw
        if (!$objects.TrimEnd().EndsWith('PASS 18 object diagnostic cases and 18 nonempty equal artifact pairs')) {
            throw 'Incomplete object macro tests'
        }
        foreach ($mode in 'c89', 'c99', 'default') {
            foreach ($case in 'recursive', 'paste', 'redefine') {
                foreach ($g in 1, 2) {
                    foreach ($text in $log, $objects) {
                        if ([regex]::Matches($text, '(?m)^' + [regex]::Escape("PASS generation $g $mode $case diagnostics and outcome") + '\r?$').Count -ne 1) {
                            throw 'Missing macro execution'
                        }
                    }
                }
                Pair (Join-Path $dir "diagnostic-tests/1/$mode/$case.exe") (Join-Path $dir "diagnostic-tests/2/$mode/$case.exe")
                foreach ($ext in 'exe', 'c1o') {
                    Pair (Join-Path $dir "diagnostic-object-tests/1/$mode/$case.$ext") (Join-Path $dir "diagnostic-object-tests/2/$mode/$case.$ext")
                }
            }
        }
    }
    elseif ($name -eq 'integers') {
        $objects = Get-Content (Join-Path $dir 'TESTS-OBJECTS.txt') -Raw
        if (!$log.TrimEnd().EndsWith('PASS equal integer conversion probe artifacts')) {
            throw 'Incomplete integer direct tests'
        }
        if (!$objects.TrimEnd().EndsWith('PASS both generations integer object review and two equal artifact pairs')) {
            throw 'Incomplete integer object tests'
        }
        foreach ($g in 1, 2) {
            if ([regex]::Matches($log, "(?m)^PASS generation $g integer narrowing cases\r?$").Count -ne 1) {
                throw 'Missing integer direct execution'
            }
            if ([regex]::Matches($objects, "(?m)^PASS generation $g 69634 integer narrowing inputs and six static initializers\r?$").Count -ne 1) {
                throw 'Missing integer object execution'
            }
        }
        Pair (Join-Path $dir 'probe1.exe') (Join-Path $dir 'probe2.exe')
        foreach ($ext in 'exe', 'c1o') {
            Pair (Join-Path $dir "object-probe1.$ext") (Join-Path $dir "object-probe2.$ext")
        }
    }
    elseif ($name -eq 'limits') {
        if (!$log.TrimEnd().EndsWith('PASS combined limits executables identical')) {
            throw 'Incomplete translation limits'
        }
        foreach ($g in 1, 2) {
            if ([regex]::Matches($log, "(?m)^PASS generation $g combined translation limits\r?$").Count -ne 1) {
                throw 'Missing limits execution'
            }
        }
        Pair (Join-Path $dir 'limits1/limits.exe') (Join-Path $dir 'limits2/limits.exe')
    }
    else {
        $gate = if ($name -eq 'facts') {
            'PASS equal implementation fact probe artifacts'
        }
        elseif ($name -eq 'multibyte') {
            'PASS both generations multibyte review and two equal artifact pairs'
        }
        else {
            'PASS both generations record search review and two equal artifact pairs'
        }
        if (!$log.TrimEnd().EndsWith($gate)) {
            throw "Incomplete probe: $name"
        }
        foreach ($g in 1, 2) {
            if ([regex]::Matches($log, "(?m)^PASS generation $g ").Count -ne 1) {
                throw 'Missing probe execution'
            }
        }
        Pair (Join-Path $dir 'probe1.exe') (Join-Path $dir 'probe2.exe')
        if ($name -ne 'facts') {
            Pair (Join-Path $dir 'probe1.c1o') (Join-Path $dir 'probe2.c1o')
        }
    }
}
"PASS $($Cases.Count) selected suites, materialized source text, execution gates and $pairs nonempty equal artifact pairs"
'This read-only audit does not independently establish the provenance of the selected compiler or runtime; use the matching seed audit for that.'
