param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$log = Get-Content (Join-Path $BuildDirectory 'SIGNED-SHIFT.txt') -Raw
$cases = @('static-negative', 'static-sign-bit', 'static-discard-high', 'enum-negative', 'case-negative', 'bound-negative', 'null-pointer', 'preprocessor-negative', 'preprocessor-sign', 'unsigned-control', 'signed-control', 'reject-negative-count', 'reject-wide-count')
foreach ($name in $cases) {
    if ([regex]::Matches($log, '(?m)^PASS C1 ' + $name + '\r?$').Count -ne 4) {
        throw 'Missing/repeated case gate'
    }
}
if (!$log.TrimEnd().EndsWith('PASS 13 shift cases in both modes and generations; 13 equal executable pairs')) {
    throw 'Missing final shift gate'
}
$pairs = 0
$rejected = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($name in $cases) {
        $reject = $name.StartsWith('reject-') -or ($mode -eq 'c99' -and !$name.EndsWith('-control'))
        $dir = Join-Path $BuildDirectory 'namespace/signed-shift'
        if ($reject) {
            foreach ($g in 1, 2) {
                $exe = Join-Path $dir "fixed-$mode-$g/$name.exe"
                $err = Join-Path $dir "fixed-$mode-$g/$name.err"
                if ((Get-Item $exe).Length -ne 0 -or [IO.File]::ReadAllText($err) -notmatch '(?m)^C1 error: ') {
                    throw 'Missing rejection evidence'
                }
                $rejected++
            }
        }
        else {
            $a = [IO.File]::ReadAllBytes((Join-Path $dir "fixed-$mode-1/$name.exe"))
            $b = [IO.File]::ReadAllBytes((Join-Path $dir "fixed-$mode-2/$name.exe"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Empty/unequal artifacts'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Artifact mismatch'
                }
            }
            $pairs++
        }
    }
}
if ($pairs -ne 13 -or $rejected -ne 26) {
    throw 'Unexpected audit totals'
}
'PASS signed shifts: 26 executions, 26 rejections and 13 equal executable pairs'