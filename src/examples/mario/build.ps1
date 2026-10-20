#requires -Version 5.1
param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '../../bootstrap-c99/build-from-seed'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'build')
)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../bootstrap-c99'))
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$compiler = Join-Path $BuildDirectory 'host/c1-stage2.exe'
$linker = Join-Path $BuildDirectory 'link-stage2.exe'
foreach ($tool in $compiler, $linker) {
    if (!(Test-Path -LiteralPath $tool)) {
        throw "Missing self-built tool: $tool"
    }
}
. "$snapshot/tools/compile.ps1"
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$saved = @{}
foreach ($name in 'C1_INCLUDE', 'C1_STANDARD', 'C1_OUTPUT', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
# Runtime source includes paths rooted at the release directory.
Push-Location ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..')))
try {
    $env:C1_INCLUDE = ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $env:C1_INCLUDE += ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    Compile $compiler (Join-Path $PSScriptRoot 'mario.c') "$OutputDirectory/mario.c1o"
    $source = "#define C1_MAIN_ARGS 1`n#include `"" + (Join-Path $snapshot 'runtime/runtime.c').Replace('\', '/') + "`"`n"
    [IO.File]::WriteAllText("$OutputDirectory/runtime.c", $source, [Text.UTF8Encoding]::new($false))
    $env:C1_STANDARD = $null
    $env:C1_RUNTIME = $null
    Compile $compiler "$OutputDirectory/runtime.c" "$OutputDirectory/runtime.c1o"
    $linkArgs = @("$OutputDirectory/mario.exe", '__c1_start', "$OutputDirectory/mario.c1o", "$OutputDirectory/runtime.c1o")
    foreach ($api in (Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json)) {
        $linkArgs += @('--import', ('__c1_win_' + $api), 'KERNEL32.dll', $api)
    }
    foreach ($api in 'GetModuleHandleA', 'GetTickCount') {
        $linkArgs += @('--import', $api, 'KERNEL32.dll', $api)
    }
    foreach ($api in 'RegisterClassExA', 'CreateWindowExA', 'DefWindowProcA', 'ShowWindow', 'UpdateWindow', 'GetMessageA', 'TranslateMessage', 'DispatchMessageA', 'PostQuitMessage', 'DestroyWindow', 'UnregisterClassA', 'LoadCursorA', 'AdjustWindowRect', 'BeginPaint', 'EndPaint', 'FillRect', 'FrameRect', 'InvalidateRect', 'SetWindowTextA', 'SendMessageA', 'MessageBoxA', 'SetWindowPos', 'GetClientRect', 'GetDC', 'ReleaseDC', 'SetTimer', 'KillTimer') {
        $linkArgs += @('--import', $api, 'USER32.dll', $api)
    }
    $linkArgs += @('--import', 'DrawTextW', 'USER32.dll', 'DrawTextW')
    foreach ($api in 'CreateSolidBrush', 'DeleteObject', 'SetBkMode', 'SetTextColor', 'TextOutA', 'CreateFontA', 'SelectObject', 'CreateCompatibleDC', 'CreateCompatibleBitmap', 'DeleteDC', 'BitBlt', 'GetPixel', 'GetDIBits') {
        $linkArgs += @('--import', $api, 'GDI32.dll', $api)
    }
    & $linker @linkArgs --subsystem windows
    if ($LASTEXITCODE -ne 0) {
        throw "Link failed: $LASTEXITCODE"
    }
    "Built $OutputDirectory/mario.exe"
}
finally {
    Pop-Location
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
