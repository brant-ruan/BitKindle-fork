param([string]$BuildDirectory = (Join-Path $PSScriptRoot '../../bootstrap-c99/build-from-seed'))
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../bootstrap-c99'))
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$compiler = Join-Path $BuildDirectory 'host/c1-stage2.exe'
$linker = Join-Path $BuildDirectory 'link-stage2.exe'
foreach ($tool in $compiler, $linker) {
    if (!(Test-Path -LiteralPath $tool)) {
        throw "Missing self-built tool: $tool" 
    } 
}
. (Join-Path $snapshot 'tools/compile.ps1')
$build = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $build | Out-Null
$saved = @{}
foreach ($name in 'C1_INCLUDE', 'C1_STANDARD', 'C1_OUTPUT', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name) 
}
try {
    $env:C1_INCLUDE = ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $env:C1_INCLUDE += ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    Compile $compiler (Join-Path $PSScriptRoot 'minicurl.c') (Join-Path $build 'minicurl.c1o')
    $runtimeSource = Join-Path $build 'runtime.c'
    $body = "#define C1_MAIN_ARGS 1`n#include `"" + (Join-Path $snapshot 'runtime/runtime-c89.c').Replace('\', '/') + "`"`n"
    [IO.File]::WriteAllText($runtimeSource, $body, [Text.UTF8Encoding]::new($false))
    $env:C1_STANDARD = $null
    $env:C1_RUNTIME = 'c89'
    Compile $compiler $runtimeSource (Join-Path $build 'runtime.c1o')
    $arguments = @((Join-Path $PSScriptRoot 'minicurl.exe'), '__c1_start', (Join-Path $build 'minicurl.c1o'), (Join-Path $build 'runtime.c1o'))
    foreach ($api in (Get-Content (Join-Path $snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json)) {
        $arguments += @('--import', ('__c1_win_' + $api), 'KERNEL32.dll', $api)
    }
    foreach ($api in 'WinHttpCrackUrl', 'WinHttpOpen', 'WinHttpSetTimeouts', 'WinHttpConnect', 'WinHttpOpenRequest', 'WinHttpSendRequest', 'WinHttpReceiveResponse', 'WinHttpQueryHeaders', 'WinHttpReadData', 'WinHttpCloseHandle') {
        $arguments += @('--import', $api, 'WINHTTP.dll', $api)
    }
    foreach ($api in 'GetLastError', 'GetStdHandle', 'WriteFile') {
        $arguments += @('--import', $api, 'KERNEL32.dll', $api) 
    }
    & $linker @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Link failed: $LASTEXITCODE" 
    }
    Get-FileHash -Algorithm SHA256 -LiteralPath $compiler, $linker, (Join-Path $PSScriptRoot 'minicurl.exe') |
        Select-Object Path, Hash | ConvertTo-Json | Set-Content (Join-Path $build 'provenance.json') -Encoding utf8
    'Built minicurl.exe with the self-hosted C89 compiler and custom PE linker.'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name]) 
    }
}
