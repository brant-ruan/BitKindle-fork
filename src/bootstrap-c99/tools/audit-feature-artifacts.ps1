param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$pairs = [Collections.Generic.List[object]]::new()
foreach ($kind in 'inline', 'compound') {
    foreach ($name in 'a.c1o', 'b.c1o', 'external.c1o', 'linked.exe') {
        $pairs.Add(@("features/$kind/objects1/$name", "features/$kind/objects2/$name"))
    }
}
foreach ($name in 'missing.c1o', 'duplicate-a.c1o', 'duplicate-b.c1o', 'private-a.c1o', 'private-b.c1o', 'private-main.c1o', 'private.exe') {
    $pairs.Add(@("features/inline/edges1/$name", "features/inline/edges2/$name"))
}
foreach ($kind in 'inttypes', 'wctype', 'wchar-string', 'restartable', 'wide-integer', 'wide-float', 'wide-time', 'wide-io', 'wide-output', 'wide-input', 'c89-addresses') {
    foreach ($extension in '.c1o', '.exe') {
        $name = $kind + $extension
        $pairs.Add(@("features/library1/$name", "features/library2/$name"))
    }
}
if ($pairs.Count -ne 37) {
    throw 'Audit inventory error'
}
foreach ($pair in $pairs) {
    $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory $pair[0]))
    $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory $pair[1]))
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw "Empty or unequal artifacts: $($pair[0])"
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw "Artifact byte mismatch: $($pair[0]) at $i"
        }
    }
    "PASS actual bytes $($pair[0]) = $($pair[1]) ($($a.Length))"
}
'PASS 37 explicitly enumerated nonempty artifact pairs verified directly; execution logs and seed provenance are separate requirements'
