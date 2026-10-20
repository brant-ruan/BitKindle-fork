param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Output,
    [ValidateSet('c89', 'c99')][string]$Standard = 'c89',
    [string]$Snapshot,
    [string]$BuildDirectory,
    [switch]$MainArguments,
    [ValidateSet('console', 'windows')][string]$Subsystem = 'console'
)
$ErrorActionPreference = 'Stop'
if (!$Snapshot) {
    $Snapshot = $PSScriptRoot
}
if (!$BuildDirectory) {
    $BuildDirectory = Join-Path $PSScriptRoot 'build'
}
$Snapshot = [IO.Path]::GetFullPath($Snapshot)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$Source = [IO.Path]::GetFullPath($Source)
$Output = [IO.Path]::GetFullPath($Output)
if (!(Test-Path -LiteralPath $Source -PathType Leaf)) {
    throw 'Source does not exist'
}
if ([IO.Path]::GetExtension($Output) -ne '.exe') {
    throw 'Output must have an .exe extension'
}
if ($Source -eq $Output) {
    throw 'Source and output must differ'
}
$compiler = Join-Path $BuildDirectory 'host/c1-stage2.exe'
$linker = Join-Path $BuildDirectory 'link-stage2.exe'
foreach ($tool in $compiler, $linker) {
    if (!(Test-Path -LiteralPath $tool)) {
        throw "Build the self-hosted toolchain first: $tool"
    }
}
. (Join-Path $Snapshot 'tools/compile.ps1')
New-Item -ItemType Directory -Force ([IO.Path]::GetDirectoryName($Output)) | Out-Null
# Keep generated sources and objects away from user-owned neighboring files.
$intermediates = Join-Path ([IO.Path]::GetDirectoryName($Output)) ('.c1-build-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $intermediates | Out-Null
$object = Join-Path $intermediates 'program.c1o'
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_INCLUDE = ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $Snapshot $_ }) -join ';')
    $env:C1_STANDARD = $Standard
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    Compile $compiler $Source $object
    if ($MainArguments) {
        $runtimeSource = Join-Path $intermediates 'runtime.c'
        $runtime = Join-Path $intermediates 'runtime.c1o'
        $runtimeHeader = if ($Standard -eq 'c89') {
            'runtime/runtime-c89.c'
        }
        else {
            'runtime/runtime.c'
        }
        $body = "#define C1_MAIN_ARGS 1`n#include `"" + (Join-Path $Snapshot $runtimeHeader).Replace('\', '/') + "`"`n"
        [IO.File]::WriteAllText($runtimeSource, $body, [Text.UTF8Encoding]::new($false))
        $env:C1_STANDARD = $null
        $env:C1_RUNTIME = if ($Standard -eq 'c89') {
            'c89'
        }
        else {
            $null
        }
        Compile $compiler $runtimeSource $runtime
    }
    else {
        $runtime = if ($Standard -eq 'c89') {
            Join-Path $BuildDirectory 'namespace/runtime-policy2/runtime.c1o'
        }
        else {
            Join-Path $BuildDirectory 'host/runtime2.c1o'
        }
    }
    if (!(Test-Path -LiteralPath $runtime)) {
        throw "Build the matching runtime first: $runtime"
    }
    $options = ((Get-Content (Join-Path $Snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    $p = Start-Process -FilePath $linker -ArgumentList ('"' + $Output + '" __c1_start "' + $object + '" "' + $runtime + '" --subsystem ' + $Subsystem + ' ' + $options) -WindowStyle Hidden -PassThru
    [void]$p.Handle
    if (!$p.WaitForExit(30000)) {
        $p.Kill()
        throw 'Link timeout'
    }
    $p.WaitForExit()
    if ($p.ExitCode) {
        throw "Link failed: $($p.ExitCode)"
    }
    "Built hosted $Standard executable: $Output"
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
