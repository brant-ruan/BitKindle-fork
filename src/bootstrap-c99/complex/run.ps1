param([Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$Rerun)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $BuildDirectory 'complex'
# Reference generators remain runnable after materialization in the build tree.
$compatDirectory = Join-Path $BuildDirectory 'base'
New-Item -ItemType Directory -Force $compatDirectory | Out-Null
[IO.File]::WriteAllText((Join-Path $compatDirectory 'ps51-compat.ps1'), [IO.File]::ReadAllText((Join-Path $root 'base/ps51-compat.ps1')), [Text.UTF8Encoding]::new($false))
if ((Test-Path $out) -and !$Rerun) {
    throw 'Use a fresh complex test directory, or explicitly rerun every suite'
}
$manifest = Get-Content "$root/SOURCES.json" -Raw | ConvertFrom-Json
foreach ($entry in $manifest) {
    if (!$entry.path.StartsWith('complex/')) {
        continue
    }
    $source = Join-Path $root $entry.path
    $path = Join-Path $out $entry.path.Substring(8)
    New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($path)) | Out-Null
    [IO.File]::WriteAllText($path, [IO.File]::ReadAllText($source), [Text.UTF8Encoding]::new($false))
}
$suites = Get-Content "$PSScriptRoot/SUITES.json" -Raw | ConvertFrom-Json
foreach ($suite in $suites) {
    $name = $suite.Name
    $log = Join-Path $out "$name.txt"
    & (Join-Path $out "test-$name.ps1") -BuildDirectory $BuildDirectory *> $log
    if (!$?) {
        throw "Complex test failed: $name"
    }
    if (!([IO.File]::ReadAllText($log).TrimEnd().EndsWith($suite.Gate))) {
        throw "Missing terminal gate: $name"
    }
    "PASS packaged complex suite $name"
}
$artifacts = @(Get-ChildItem $out -Recurse -File | Where-Object { $_.Extension -in '.exe', '.c1o', '.err', '.stderr', '.stdout', '.txt' } | Sort-Object FullName | ForEach-Object {
        [ordered]@{path = $_.FullName.Substring($out.Length + 1).Replace('\', '/'); bytes = $_.Length; sha256 = (Get-FileHash $_.FullName).Hash }
    })
$artifacts | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $out 'ARTIFACTS.json')
"PASS $($suites.Count) packaged complex/compiler/runtime suites"
