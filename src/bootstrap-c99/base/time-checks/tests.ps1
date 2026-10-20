$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    . (Join-Path $PSScriptRoot 'history-tests.ps1')
}
finally {
    $env:C1_INCLUDE = $savedInclude
}
