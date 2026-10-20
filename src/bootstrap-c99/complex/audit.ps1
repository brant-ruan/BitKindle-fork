param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $BuildDirectory 'complex'
$manifest = Get-Content "$root/SOURCES.json" -Raw | ConvertFrom-Json
foreach ($entry in $manifest) {
    if (!$entry.path.StartsWith('complex/')) {
        continue
    }
    if ([IO.File]::ReadAllText((Join-Path $out $entry.path.Substring(8))) -cne [IO.File]::ReadAllText((Join-Path $root $entry.path))) {
        throw 'Materialized complex source changed'
    }
}
$suites = Get-Content "$PSScriptRoot/SUITES.json" -Raw | ConvertFrom-Json
$outer = Get-Content (Join-Path $BuildDirectory 'COMPLEX.txt') -Raw
if (!$outer.TrimEnd().EndsWith("PASS $($suites.Count) packaged complex/compiler/runtime suites")) {
    throw 'Missing outer complex completion'
}
foreach ($suite in $suites) {
    $log = Get-Content (Join-Path $out ($suite.Name + '.txt')) -Raw
    if (!$log.TrimEnd().EndsWith($suite.Gate)) {
        throw "Missing suite completion: $($suite.Name)"
    }
    if ([regex]::Matches($outer, '(?m)^PASS packaged complex suite ' + [regex]::Escape($suite.Name) + '\r?$').Count -ne 1) {
        throw 'Missing or duplicated suite'
    }
}
$artifacts = Get-Content (Join-Path $out 'ARTIFACTS.json') -Raw | ConvertFrom-Json
if (@($artifacts | Where-Object { $_.path -like '*.exe' }).Count -lt 100) {
    throw 'Incomplete executable inventory'
}
foreach ($entry in $artifacts) {
    $path = Join-Path $out $entry.path
    if ((Get-Item $path).Length -ne $entry.bytes -or (Get-FileHash $path).Hash -ne $entry.sha256) {
        throw "Changed complex artifact: $($entry.path)"
    }
}
# Independently recheck the final integrated compiler/runtime pairs and all
# full-runtime clients. The original suite checks cover standalone pair shapes.
function Pair($a, $b) {
    $x = [IO.File]::ReadAllBytes($a)
    $y = [IO.File]::ReadAllBytes($b)
    if (!$x.Length -or $x.Length -ne $y.Length) {
        throw 'Empty or unequal integrated pair'
    }
    for ($i = 0; $i -lt $x.Length; $i++) {
        if ($x[$i] -ne $y[$i]) {
            throw 'Integrated byte mismatch'
        }
    }
}
foreach ($stem in 'compiler', 'runtime', 'c1-stage') {
    $ext = if ($stem -eq 'c1-stage') {
        'exe'
    }
    else {
        'c1o'
    }
    Pair "$BuildDirectory/host/$($stem)1.$ext" "$BuildDirectory/host/$($stem)2.$ext"
}
$clients = @(Get-ChildItem "$out/hosted-clients" -Filter '*1.exe' -File)
if ($clients.Count -ne 24) {
    throw 'Expected 24 integrated client pairs'
}
foreach ($file in $clients) {
    $name = $file.BaseName.Substring(0, $file.BaseName.Length - 1)
    foreach ($ext in 'exe', 'c1o') {
        Pair "$out/hosted-clients/$($name)1.$ext" "$out/hosted-clients/$($name)2.$ext"
    }
}
"PASS $($suites.Count) packaged suite gates, $($artifacts.Count) checked artifact hashes, 51 independently compared integrated pairs"
