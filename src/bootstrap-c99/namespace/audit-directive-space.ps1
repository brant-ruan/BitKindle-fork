param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$log = Get-Content (Join-Path $BuildDirectory 'DIRECTIVE-SPACE.txt')
$root = Join-Path $BuildDirectory 'namespace/directive-space'
$invalid = @('define_vt', 'define_ff', 'if_vt', 'null_ff', 'include_trailing')
$valid = @('before_hash', 'ordinary', 'comment', 'literal', 'include_header', 'skipped', 'following_line')
$pairs = 0
foreach ($mode in 'c89', 'c99') {
    foreach ($name in ($invalid + $valid)) {
        $reject = $name -in $invalid
        foreach ($g in 1, 2) {
            $suffix = if ($reject) {
                'rejected with diagnostic and no binary'
            }
            else {
                'runtime'
            }
            $gate = "PASS $mode generation $g $name $suffix"
            if (@($log | Where-Object { $_ -ceq $gate }).Count -ne 1) {
                throw "Missing/repeated gate $gate"
            }
            $err = [IO.File]::ReadAllText((Join-Path $root "$mode-$g/$name.stderr"))
            if ($reject) {
                if (!$err.Contains('invalid whitespace in preprocessing directive')) {
                    throw 'Wrong whitespace diagnostic'
                }
            }
            elseif ($err.Length) {
                throw 'Unexpected diagnostic'
            }
        }
        $a = [IO.File]::ReadAllBytes((Join-Path $root "$mode-1/$name.exe"))
        $b = [IO.File]::ReadAllBytes((Join-Path $root "$mode-2/$name.exe"))
        if ($reject) {
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
'PASS directive whitespace: 20 diagnosed rejections, 28 executions, 14 equal executable pairs'