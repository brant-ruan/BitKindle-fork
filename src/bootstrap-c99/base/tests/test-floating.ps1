$savedFloatingInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$floatingFixtureRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).Replace('\', '/') + '/'
function FloatingCheck([string]$Name, [string]$Source, [int]$Result = 0, [bool]$Reject = $false, [string]$RuntimeInput = '', [string]$ExpectedOutput = '') {
    # Resolve the fixed fixture include prefix to this checkout at test time.
    Check $Name ($Source.Replace('src/bootstrap-c99/base/', $floatingFixtureRoot)) $Result $Reject $RuntimeInput $ExpectedOutput
}
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    . (Join-Path $PSScriptRoot 'test-floating-runtime.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-static.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-ice.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-types.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-volatile.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-boundary.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-header.ps1')
    . (Join-Path $PSScriptRoot 'test-floating-env.ps1')
    FloatingCheck 'fp-independent-abi' ([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'floating-abi.c')))
    FloatingCheck 'fp-constant-leaves' ([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'floating-eval.c')))
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedFloatingInclude, 'Process')
}