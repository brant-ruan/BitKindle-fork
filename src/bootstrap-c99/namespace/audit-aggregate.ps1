param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$log = Get-Content (Join-Path $BuildDirectory 'AGGREGATE.txt')
$root = Join-Path $BuildDirectory 'namespace/aggregate'
$invalid = @('array', 'call', 'record', 'bitfield', 'union', 'nestedcopy')
$valid = @('scalar', 'recordcopy', 'constant', 'elided')
$pairs = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($name in ($invalid + $valid)) {
        $rejected = $mode -eq 'c89' -and $name -in $invalid
        foreach ($g in 1, 2) {
            $gate = if ($rejected) {
                "PASS c89 generation $g $name rejected with diagnostic and no binary"
            }
            else {
                "PASS $mode generation $g $name runtime"
            }
            if (@($log | Where-Object { $_ -ceq $gate }).Count -ne 1) {
                throw "Missing/repeated gate $gate"
            }
            $err = [IO.File]::ReadAllText((Join-Path $root "$mode-$g/$name.stderr"))
            if ($rejected) {
                if (!$err.Contains('C1 error:')) {
                    throw 'Missing diagnostic'
                }
            }
            elseif ($err.Length) {
                throw 'Unexpected diagnostic'
            }
        }
        $a = [IO.File]::ReadAllBytes((Join-Path $root "$mode-1/$name.exe"))
        $b = [IO.File]::ReadAllBytes((Join-Path $root "$mode-2/$name.exe"))
        if ($rejected) {
            if ($a.Length -or $b.Length) {
                throw 'Invalid source emitted bytes'
            }
        }
        else {
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Empty/unequal artifacts'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Unequal artifact bytes'
                }
            }
            $pairs++
        }
    }
}
if ($pairs -ne 14) {
    throw 'Wrong pair count'
}
'PASS aggregate constraints: 12 rejected compilations, 28 executions, 14 equal executable pairs'
$log = Get-Content (Join-Path $BuildDirectory 'CHECKPOINT.txt')
foreach ($g in 1, 2) {
    if (@($log | Where-Object { $_ -ceq "PASS fixed generation $g checkpoint test exit 0" }).Count -ne 1) {
        throw 'Missing checkpoint gate'
    }
}
foreach ($ext in 'c1o', 'exe') {
    $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "namespace/checkpoint/fixed1.$ext"))
    $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "namespace/checkpoint/fixed2.$ext"))
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw 'Empty/unequal checkpoint artifacts'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Checkpoint artifacts differ'
        }
    }
}
'PASS checkpoint executions and two equal artifact pairs'