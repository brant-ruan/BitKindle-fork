param([string]$BuildDirectory = (Join-Path $PSScriptRoot 'build'))
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'tools/audit-source-paths.ps1') -Snapshot $PSScriptRoot
if (!$?) {
    throw 'Foreign snapshot source dependency'
}
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
New-Item -ItemType Directory -Force $BuildDirectory | Out-Null
$savedOutput = $env:C1_OUTPUT
$savedInclude = $env:C1_INCLUDE
$savedStandard = $env:C1_STANDARD
$savedRuntime = $env:C1_RUNTIME
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_STANDARD = $null
    $env:C1_RUNTIME = $null
    & (Join-Path $PSScriptRoot 'base/build.ps1') -BuildDirectory (Join-Path $BuildDirectory 'base') *> (Join-Path $BuildDirectory 'BASE-VERIFICATION.txt')
    if (-not $?) {
        throw 'Seed/base build failed; see BASE-VERIFICATION.txt'
    }
    & (Join-Path $PSScriptRoot 'base/audit.ps1') -BuildDirectory (Join-Path $BuildDirectory 'base') -VerificationLog (Join-Path $BuildDirectory 'BASE-VERIFICATION.txt') *> (Join-Path $BuildDirectory 'BASE-AUDIT.txt')
    if (-not $?) {
        throw 'Base audit failed'
    }
    & (Join-Path $PSScriptRoot 'build-toolchain.ps1') -BaseBuild (Join-Path $BuildDirectory 'base') -BuildDirectory $BuildDirectory
    if (-not $?) {
        throw 'Object toolchain build failed'
    }
    & (Join-Path $PSScriptRoot 'features/run.ps1') -BuildDirectory $BuildDirectory -Assembler (Join-Path $BuildDirectory 'base/a1-stage2.exe') *> (Join-Path $BuildDirectory 'FEATURES.txt')
    if (-not $?) {
        throw 'Core language features failed'
    }
    & (Join-Path $PSScriptRoot 'features/library/run.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'LIBRARY.txt')
    if (-not $?) {
        throw 'Cumulative library suites failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/run.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'NAMESPACE.txt')
    if (!$?) {
        throw 'C89 namespace integration failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-hygiene.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'HYGIENE.txt')
    if (!$?) {
        throw 'Public header macro hygiene failed'
    }
    & (Join-Path $PSScriptRoot 'diagnostics/run.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'DIAGNOSTICS.txt')
    if (!$?) {
        throw 'C89 diagnostic and preprocessing tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-locale.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'LOCALE.txt')
    if (!$?) {
        throw 'Locale layout and namespace tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-numeric.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'NUMERIC.txt')
    if (!$?) {
        throw 'Numeric policy tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-math-policy.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'MATH-POLICY.txt')
    if (!$?) {
        throw 'Math policy tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-dbcs.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'DBCS.txt')
    if (!$?) {
        throw 'Header path tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-checkpoint.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'CHECKPOINT.txt')
    if (!$?) {
        throw 'Checkpoint state tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-aggregate.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'AGGREGATE.txt')
    if (!$?) {
        throw 'Aggregate constant tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-return.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'RETURN.txt')
    if (!$?) {
        throw 'Return tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-directive-space.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'DIRECTIVE-SPACE.txt')
    if (!$?) {
        throw 'Directive whitespace tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-clock.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'CLOCK.txt')
    if (!$?) {
        throw 'Clock fallback tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-signed-shift.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'SIGNED-SHIFT.txt')
    if (!$?) {
        throw 'Signed shift tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-ucn.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'UCN.txt')
    if (!$?) {
        throw 'UCN tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-character-sign.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'CHARACTER-SIGN.txt')
    if (!$?) {
        throw 'Character sign tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-constant-comma.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'CONSTANT-COMMA.txt')
    if (!$?) {
        throw 'Constant comma tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-empty-unit.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'EMPTY-UNIT.txt')
    if (!$?) {
        throw 'Empty translation unit tests failed'
    }
    & (Join-Path $PSScriptRoot 'namespace/test-vla.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'VLA.txt')
    if (!$?) {
        throw 'VLA tests failed'
    }
    & (Join-Path $PSScriptRoot 'complex/run.ps1') -BuildDirectory $BuildDirectory *> (Join-Path $BuildDirectory 'COMPLEX.txt')
    if (!$?) {
        throw 'Complex/current runtime suites failed'
    }
    foreach ($g in 1, 2) {
        & (Join-Path $PSScriptRoot 'features/run-full.ps1') -Compiler (Join-Path $BuildDirectory ('host/c1-stage' + $g + '.exe')) -Assembler (Join-Path $BuildDirectory 'base/a1-stage2.exe') -ScratchDirectory (Join-Path $BuildDirectory ('regression' + $g)) *> (Join-Path $BuildDirectory ('REGRESSION-' + $g + '.txt'))
        if (-not $?) {
            throw "Final compiler regression $g failed"
        }
    }
    'PASS complete seed-to-object-toolchain build and final regressions'
}
finally {
    $env:C1_OUTPUT = $savedOutput
    $env:C1_INCLUDE = $savedInclude
    $env:C1_STANDARD = $savedStandard
    $env:C1_RUNTIME = $savedRuntime
}
