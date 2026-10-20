param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$Log)
$ErrorActionPreference = 'Stop'
if (!$Log) {
    $Log = Join-Path $BuildDirectory 'DIAGNOSTICS.txt'
}
$body = Get-Content -LiteralPath $Log -Raw
foreach ($gate in @(
        'PASS 40 preprocessing cases and 14 nonempty equal executable pairs',
        'PASS both mode matrices and equal compiler-generation artifacts',
        'PASS 228 diagnostic cases and 111 nonempty equal executable pairs',
        'PASS 228 object diagnostic cases and 222 nonempty equal artifact pairs',
        'PASS 78 diagnostic cases and 24 nonempty equal executable pairs',
        'PASS 78 object diagnostic cases and 48 nonempty equal artifact pairs',
        'PASS all C89 diagnostic and preprocessing gates'
    )) {
    if (!$body.Contains($gate)) {
        throw "Missing diagnostic gate: $gate"
    }
}
$pairs = 0
foreach ($spec in @(@('tests', 14), @('matrix', 2), @('diagnostic-tests', 111), @('diagnostic-object-tests', 222), @('skipped-tests', 24), @('skipped-object-tests', 48))) {
    $one = Join-Path $BuildDirectory ($spec[0] + '/1')
    $two = Join-Path $BuildDirectory ($spec[0] + '/2')
    $count = 0
    foreach ($file in Get-ChildItem -LiteralPath $one -Recurse -File | Where-Object { $_.Extension -in '.exe', '.c1o' }) {
        $relative = $file.FullName.Substring(([IO.Path]::GetFullPath($one)).Length + 1)
        $a = [IO.File]::ReadAllBytes($file.FullName)
        $b = [IO.File]::ReadAllBytes((Join-Path $two $relative))
        if ($a.Length -ne $b.Length) {
            throw "Unequal artifact length: $relative"
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Unequal artifact bytes: $relative"
            }
        }
        if ($a.Length) {
            $count++
        }
    }
    if ($count -ne $spec[1]) {
        throw "Wrong nonempty artifact count for $($spec[0]): $count"
    }
    $pairs += $count
}
"PASS $pairs nonempty diagnostic artifact pairs and required execution gates"
