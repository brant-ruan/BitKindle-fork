param(
    [Parameter(Mandatory = $true)][string]$Snapshot,
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$ScratchDirectory,
    [ValidateSet('headers', 'macros', 'multibyte', 'search', 'facts', 'integers', 'limits')][string[]]$Cases = @('headers', 'macros', 'multibyte', 'search', 'facts', 'integers', 'limits')
)
$ErrorActionPreference = 'Stop'
$Snapshot = [IO.Path]::GetFullPath($Snapshot)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
if (Test-Path $ScratchDirectory) {
    throw 'Use a new scratch directory to preserve prior results'
}
foreach ($g in 1, 2) {
    foreach ($path in "host/c1-stage$g.exe", "link-stage$g.exe", "namespace/runtime-policy$g/runtime.c1o") {
        if (!(Test-Path (Join-Path $BuildDirectory $path))) {
            throw "Missing self-built prerequisite: $path"
        }
    }
}
New-Item -ItemType Directory -Path $ScratchDirectory | Out-Null
foreach ($name in $Cases) {
    $dir = Join-Path $ScratchDirectory $name
    New-Item -ItemType Directory -Path $dir | Out-Null
    # Only copy source TEXT. Every binary is freshly emitted by the selected tools.
    foreach ($file in Get-ChildItem (Join-Path $PSScriptRoot "templates/$name") -File) {
        if ($file.Extension -notin '.c', '.ps1') {
            throw 'Unexpected template file'
        }
        [IO.File]::WriteAllText((Join-Path $dir $file.Name), (Get-Content $file.FullName -Raw), [Text.UTF8Encoding]::new($false))
    }
    & (Join-Path $dir 'test.ps1') -Snapshot $Snapshot -BuildDirectory $BuildDirectory *> (Join-Path $dir 'TESTS.txt')
    if (!$?) {
        throw "Supplemental case failed: $name"
    }
    if ($name -eq 'macros') {
        & (Join-Path $dir 'test.ps1') -Snapshot $Snapshot -BuildDirectory $BuildDirectory -ObjectMode *> (Join-Path $dir 'TESTS-OBJECTS.txt')
        if (!$?) {
            throw 'Macro object cases failed'
        }
    }
    if ($name -eq 'integers') {
        & (Join-Path $dir 'test-objects.ps1') -Snapshot $Snapshot -BuildDirectory $BuildDirectory *> (Join-Path $dir 'TESTS-OBJECTS.txt')
        if (!$?) {
            throw 'Integer object tests failed'
        }
    }
    "PASS supplemental $name"
}
'PASS selected supplemental test suites'
