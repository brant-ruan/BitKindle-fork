param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$log = Get-Content (Join-Path $BuildDirectory 'RETURN.txt')
$root = Join-Path $BuildDirectory 'namespace/bare-return'
$pairs = 0
$rejections = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($name in 'int', 'double', 'int_pointer', 'struct_Small', 'struct_Large', 'implicit', 'void', 'void_value') {
        $reject = $name -eq 'void_value' -or ($mode -eq 'c99' -and $name -ne 'void')
        foreach ($g in 1, 2) {
            $suffix = if ($reject) {
                'diagnosed rejection'
            }
            else {
                'discarded return and side effects'
            }
            $gate = "PASS fixed $mode generation $g $name $suffix"
            if (@($log | Where-Object { $_ -ceq $gate }).Count -ne 1) {
                throw "Missing/repeated gate $gate"
            }
            $err = [IO.File]::ReadAllText((Join-Path $root "fixed-$mode-$g/$name.stderr"))
            if ($reject) {
                if (!$err.Contains('C1 error:')) {
                    throw 'Missing return diagnostic'
                }
                $rejections++
            }
            elseif ($err.Length) {
                throw 'Unexpected diagnostic'
            }
        }
        $a = [IO.File]::ReadAllBytes((Join-Path $root "fixed-$mode-1/$name.exe"))
        $b = [IO.File]::ReadAllBytes((Join-Path $root "fixed-$mode-2/$name.exe"))
        if ($reject) {
            if ($a.Length -or $b.Length) {
                throw 'Rejected return emitted binary'
            }
        }
        else {
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Empty/unequal return artifacts'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Return artifact mismatch'
                }
            }
            $pairs++
        }
    }
}
if ($pairs -ne 8 -or $rejections -ne 16) {
    throw 'Return audit case count mismatch'
}
'PASS return rules: 16 executions, 16 diagnosed rejections, eight equal executable pairs'