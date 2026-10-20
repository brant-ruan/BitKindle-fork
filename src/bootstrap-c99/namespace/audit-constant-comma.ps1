param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/constant-comma'
$log = Get-Content (Join-Path $BuildDirectory 'CONSTANT-COMMA.txt') -Raw
$names = @('enum', 'static', 'pointer-static', 'pointer-auto', 'pointer-call', 'sizeof', 'runtime', 'short-overflow', 'evaluated')
$pairs = 0
$runs = 0
$rejected = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($name in $names) {
        $reject = $name -eq 'evaluated' -or ($mode -eq 'c89' -and $name -in @('pointer-auto', 'pointer-call'))
        foreach ($g in 1, 2) {
            $path = "$root/fixed-$mode-$g/$name"
            $bytes = [IO.File]::ReadAllBytes("$path.exe")
            $err = Get-Content "$path.err" -Raw
            $warning = $err -match 'C99 extension in C89 mode: unevaluated comma in constant expression'
            $want = $mode -eq 'c89' -and $name -in @('enum', 'static', 'pointer-static')
            if ([bool]$warning -ne [bool]$want) {
                throw "Wrong warning: $mode/$g/$name"
            }
            if ($reject) {
                if ($bytes.Length -ne 0 -or $err -notmatch '(?m)^C1 error: ') {
                    throw 'Rejected artifact is not empty or lacks error'
                }
                $rejected++
            }
            else {
                if ($bytes.Length -lt 1024 -or $bytes[0] -ne 77 -or $bytes[1] -ne 90) {
                    throw 'Invalid executable'
                }
                $runs++
            }
        }
        if (!$reject) {
            $a = [IO.File]::ReadAllBytes("$root/fixed-$mode-1/$name.exe")
            $b = [IO.File]::ReadAllBytes("$root/fixed-$mode-2/$name.exe")
            if ($a.Length -ne $b.Length) {
                throw 'Length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Byte mismatch'
                }
            }
            $pairs++
        }
    }
}
foreach ($name in $names) {
    if ([regex]::Matches($log, '(?m)^PASS C1 ' + [regex]::Escape($name) + '\r?$').Count -ne 4) {
        throw 'Missing case gate'
    }
}
if ($pairs -ne 14 -or $runs -ne 28 -or $rejected -ne 8 -or !$log.TrimEnd().EndsWith('PASS 36 comma mode and diagnostic checks')) {
    throw 'Comma totals mismatch'
}
'PASS constant commas: 28 executions, eight rejections, 14 identical pairs and 36 diagnostic checks'
