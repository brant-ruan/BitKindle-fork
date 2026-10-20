param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace'
$pairs = [Collections.Generic.List[object]]::new()
foreach ($name in 'runtime.c1o', 'collision.c1o', 'collision.exe', 'addresses.c1o', 'addresses.exe') {
    $pairs.Add(@("runtime-policy1/$name", "runtime-policy2/$name"))
}
foreach ($name in 'app.c1o', 'normal.c1o', 'isolated.c1o', 'normal.exe', 'isolated.exe') {
    $pairs.Add(@("common1/$name", "common2/$name"))
}
foreach ($name in 'assert', 'ctype', 'errno', 'float', 'limits', 'locale', 'math', 'setjmp', 'signal', 'stdarg', 'stddef', 'stdio', 'stdlib', 'string', 'time') {
    foreach ($ext in 'c1o', 'exe') {
        $pairs.Add(@("headers1/$name.$ext", "headers2/$name.$ext"))
    }
}
foreach ($pair in $pairs) {
    $a = [IO.File]::ReadAllBytes((Join-Path $root $pair[0]))
    $b = [IO.File]::ReadAllBytes((Join-Path $root $pair[1]))
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw "Namespace artifact length mismatch: $($pair[0])"
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw "Namespace artifact bytes mismatch: $($pair[0])"
        }
    }
}
$log = Get-Content (Join-Path $BuildDirectory 'NAMESPACE.txt')
foreach ($g in 1, 2) {
    foreach ($line in "PASS generation $g runtime namespace collision", "PASS generation $g runtime namespace addresses", "PASS generation $g normal common isolation control exit 1", "PASS generation $g isolated common isolation control exit 0") {
        if (@($log | Where-Object { $_ -eq $line }).Count -ne 1) {
            throw "Missing unique gate: $line"
        }
    }
    if (@($log | Where-Object { $_ -like "PASS generation $g isolated header *" }).Count -ne 15) {
        throw "Missing header checks: $g"
    }
}
if ($log[-1] -ne 'PASS integrated C89 runtime isolation, common controls, public headers and export audit') {
    throw 'Missing namespace final gate'
}
& (Join-Path $PSScriptRoot 'audit-runtime-symbols.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'C89 runtime export audit failed'
}
'PASS 40 nonempty namespace artifact pairs byte-identical and execution gates present'
