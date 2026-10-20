param([Parameter(Mandatory = $true)][string]$Snapshot)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($Snapshot)
$name = Split-Path $root -Leaf
$manifest = Get-Content (Join-Path $root 'SOURCES.json') -Raw | ConvertFrom-Json
$checked = 0
foreach ($entry in $manifest) {
    if ($entry.path -notmatch '\.(c|h|ps1)$') {
        continue
    }
    $source = Get-Content (Join-Path $root $entry.path) -Raw
    foreach ($match in [regex]::Matches($source, 'src/bootstrap-[a-z0-9-]+')) {
        if ($match.Value -cne "src/$name") {
            throw "Foreign snapshot dependency in $($entry.path): $($match.Value)"
        }
    }
    $checked++
}
"PASS $checked manifest C, header and PowerShell inputs reference no foreign snapshot root"
