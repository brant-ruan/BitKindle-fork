param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$lines = Get-Content (Join-Path $BuildDirectory 'DBCS.txt')
foreach ($g in 1, 2) {
    foreach ($pattern in "^PASS generation $g ACP [0-9]+ non-ASCII absolute and nested-relative header paths$", "^PASS generation $g ACP [0-9]+ C1_INCLUDE non-ASCII directory; DBCS-trail-case=(True|False)$") {
        if (@($lines | Where-Object { $_ -match $pattern }).Count -ne 1) {
            throw "Missing/repeated DBCS gate $pattern"
        }
    }
}
$dir = Join-Path $BuildDirectory 'namespace/dbcs'
foreach ($name in 'acp', 'probe', 'envprobe') {
    foreach ($ext in 'c1o', 'exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $dir "$($name)1.$ext"))
        $b = [IO.File]::ReadAllBytes((Join-Path $dir "$($name)2.$ext"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Empty/unequal path artifacts'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw 'Path artifact bytes differ'
            }
        }
    }
}
'PASS header paths four execution gates and six equal artifact pairs'