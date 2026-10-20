param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory, [Parameter(Mandatory = $true)][string]$ScratchDirectory)
$ErrorActionPreference = 'Stop'
$Snapshot = [IO.Path]::GetFullPath($Snapshot)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
if (Test-Path $ScratchDirectory) {
    throw 'Use a fresh scratch directory'
}
foreach ($name in 'characters', 'layout', 'pointers', 'rounding', 'math', 'signals') {
    $dir = Join-Path $ScratchDirectory $name
    New-Item -ItemType Directory -Force $dir | Out-Null
    foreach ($file in Get-ChildItem (Join-Path $PSScriptRoot "templates/$name") -File) {
        if ($file.Extension -notin '.c', '.ps1') {
            throw 'Non-source template'
        }
        [IO.File]::WriteAllText((Join-Path $dir $file.Name), (Get-Content $file.FullName -Raw), [Text.UTF8Encoding]::new($false))
    }
    & (Join-Path $dir 'test.ps1') -Snapshot $Snapshot -BuildDirectory $BuildDirectory *> (Join-Path $dir 'TESTS.txt')
    if (!$?) {
        throw "Probe failed: $name"
    }
    if (Test-Path (Join-Path $dir 'test-objects.ps1')) {
        & (Join-Path $dir 'test-objects.ps1') -Snapshot $Snapshot -BuildDirectory $BuildDirectory *> (Join-Path $dir 'TESTS-OBJECTS.txt')
        if (!$?) {
            throw "Object probe failed: $name"
        }
    }
    "PASS implementation suite $name"
}
'PASS all six implementation suites'
