param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory, [Parameter(Mandatory = $true)][string]$ScratchDirectory)
$ErrorActionPreference = 'Stop'
$Snapshot = [IO.Path]::GetFullPath($Snapshot)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
if (Test-Path $ScratchDirectory) {
    throw 'Use a fresh scratch directory'
}
foreach ($name in 'preprocessor', 'rescan', 'floating') {
    $dir = Join-Path $ScratchDirectory $name
    New-Item -ItemType Directory -Force $dir | Out-Null
    foreach ($file in Get-ChildItem (Join-Path $PSScriptRoot "templates/$name") -File) {
        if ($file.Extension -notin '.c', '.ps1') {
            throw 'Non-source template'
        }
        [IO.File]::WriteAllText((Join-Path $dir $file.Name), [IO.File]::ReadAllText($file.FullName))
    }
    & (Join-Path $dir 'test.ps1') -Snapshot $Snapshot -BuildDirectory $BuildDirectory *> (Join-Path $dir 'TESTS.txt')
    if (!$?) {
        throw "Suite failed: $name"
    }
    "PASS review suite $name"
}
'PASS all three review suites'
