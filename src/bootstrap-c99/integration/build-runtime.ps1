param([int]$Generation = 1, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../tools/compile.ps1')
$root = Join-Path $BuildDirectory ('integration/runtime' + $Generation)
New-Item -ItemType Directory -Force $root | Out-Null
$compiler = Join-Path $BuildDirectory ('host/c1-stage' + $Generation + '.exe')
$savedInclude = $env:C1_INCLUDE
$savedOutput = $env:C1_OUTPUT
try {
    $env:C1_INCLUDE = (@('components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { [IO.Path]::GetFullPath('src/bootstrap-c99/' + $_) }) -join ';'
    $env:C1_OUTPUT = 'object'
    foreach ($kind in @('void', 'args')) {
        $text = if ($kind -eq 'args') {
            "#define C1_MAIN_ARGS 1`n"
        }
        else {
            ''
        }
        $text += '#include "src/bootstrap-c99/runtime/runtime.c"'
        $source = Join-Path $root ('runtime-' + $kind + '.c')
        [IO.File]::WriteAllText($source, $text, [Text.Encoding]::ASCII)
        Compile $compiler $source (Join-Path $root ('runtime-' + $kind + '.c1o'))
    }
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $env:C1_OUTPUT = $savedOutput
}
