#requires -Version 5.1
param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$release = [IO.Path]::GetFullPath((Join-Path $snapshot '../..'))
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$saved = @{}
foreach ($name in 'C1_INCLUDE', 'C1_OUTPUT', 'C1_STANDARD', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
$oldDirectory = [Environment]::CurrentDirectory
Push-Location $release
try {
    [Environment]::CurrentDirectory = $release
    $env:C1_INCLUDE = $null
    $env:C1_OUTPUT = $null
    $env:C1_STANDARD = $null
    $env:C1_RUNTIME = $null
    New-Item -ItemType Directory -Force $BuildDirectory | Out-Null
    & "$snapshot/tools/audit-source-paths.ps1" -Snapshot $snapshot
    if (!$?) {
        throw 'Source path check failed'
    }
    & "$snapshot/base/build.ps1" -BuildDirectory "$BuildDirectory/base" -SkipTests
    if (!$?) {
        throw 'Base build failed'
    }
    & "$snapshot/build-toolchain.ps1" -BaseBuild "$BuildDirectory/base" -BuildDirectory $BuildDirectory -SkipTests
    if (!$?) {
        throw 'Final toolchain build failed'
    }
    # The public C89 driver expects this runtime at its normal full-build path.
    . "$snapshot/tools/compile.ps1"
    $runtimeDirectory = Join-Path $BuildDirectory 'namespace/runtime-policy2'
    New-Item -ItemType Directory -Force $runtimeDirectory | Out-Null
    $env:C1_INCLUDE = ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $env:C1_STANDARD = $null
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = 'c89'
    Compile "$BuildDirectory/host/c1-stage2.exe" "$snapshot/runtime/runtime-c89.c" "$runtimeDirectory/runtime.c1o"
    'BUILT C89 and C99 user-program runtimes; all regression suites skipped'
}
finally {
    [Environment]::CurrentDirectory = $oldDirectory
    Pop-Location
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
