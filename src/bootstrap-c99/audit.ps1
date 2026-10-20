param([string]$BuildDirectory = (Join-Path $PSScriptRoot 'build'), [string]$VerificationLog = (Join-Path $PSScriptRoot 'VERIFICATION.txt'), [switch]$Smoke)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'tools/audit-source-paths.ps1') -Snapshot $PSScriptRoot
if (!$?) {
    throw 'Foreign snapshot source dependency'
}
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
& (Join-Path $PSScriptRoot 'namespace/audit-artifacts.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Namespace artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/test-hygiene.ps1') -BuildDirectory $BuildDirectory -AuditOnly
if (!$?) {
    throw 'Header hygiene artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-locale.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Locale artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-numeric.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Numeric artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-math-policy.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Math policy artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-dbcs.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Header path audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-aggregate.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Aggregate and checkpoint audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-return.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Return artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-directive-space.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Directive whitespace artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-clock.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Clock artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-signed-shift.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Signed shift artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-ucn.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'UCN artifact audit failed'
}
& (Join-Path $PSScriptRoot 'namespace/audit-character-sign.ps1') -BuildDirectory $BuildDirectory
if (!$?) {
    throw 'Character sign artifact audit failed'
}
$pairs = [Collections.Generic.List[object]]::new()
foreach ($kind in @('identifier', 'object', 'host')) {
    $pairs.Add(@(($kind + '/c1-stage1.exe'), ($kind + '/c1-stage2.exe')))
}
$pairs.Add(@('link-stage1.exe', 'link-stage2.exe'))
foreach ($stem in @('compiler', 'runtime')) {
    $pairs.Add(@(('host/' + $stem + '1.c1o'), ('host/' + $stem + '2.c1o')))
}
foreach ($kind in @('objects', 'imports', 'runtime')) {
    foreach ($f in Get-ChildItem (Join-Path $BuildDirectory ('integration/' + $kind + '1')) -File | Where-Object { $_.Extension -in '.c1o', '.exe' }) {
        $pairs.Add(@(('integration/' + $kind + '1/' + $f.Name), ('integration/' + $kind + '2/' + $f.Name)))
    }
}
if ($pairs.Count -ne 62) {
    throw 'Unexpected artifact count'
}
foreach ($pair in $pairs) {
    $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory $pair[0]))
    $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory $pair[1]))
    if ($pair[0] -eq 'integration/objects1/syntax-rejected.c1o') {
        if ($a.Length -or $b.Length) {
            throw 'Rejected syntax published object bytes'
        }
    }
    elseif (!$a.Length -or $a.Length -ne $b.Length) {
        throw "Empty or unequal length: $($pair[0])"
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw "Bytes: $($pair[0])"
        }
    }
}
'PASS 62 artifact pairs byte-identical'
$log = [IO.File]::ReadAllText([IO.Path]::GetFullPath($VerificationLog))
foreach ($check in @(@('PASS compiled object ', 22), @('PASS data entry rejected', 2), @('PASS object syntax rejected', 2), @('PASS imported C program ', 8), @('PASS invalid import options ', 6), @('PASS hosted object runtime ', 12))) {
    if (([regex]::Matches($log, '(?m)^' + [regex]::Escape($check[0]))).Count -ne $check[1]) {
        throw "Integration total: $($check[0])"
    }
}
if (-not $log.Contains('PASS object toolchain self-compilation and both generations integration')) {
    throw 'Missing toolchain final gate'
}
'PASS 26 integration checks per generation'
if (-not $Smoke) {
    $features = [IO.File]::ReadAllText((Join-Path $BuildDirectory 'FEATURES.txt'))
    if (([regex]::Matches($features, '(?m)^PASS C1 ')).Count -ne 492) {
        throw 'Core language feature count mismatch'
    }
    if (([regex]::Matches($features, '(?m)^246 core language checks passed\.')).Count -ne 2) {
        throw 'Missing generation feature totals'
    }
    if (([regex]::Matches($features, '(?m)^PASS identical ')).Count -ne 15) {
        throw 'Feature object/executable equality count mismatch'
    }
    if (!$features.TrimEnd().EndsWith('PASS both generations core language and linking features')) {
        throw 'Missing feature final gate'
    }
    'PASS 246 core language checks per generation and 15 feature artifact pairs'
    $library = [IO.File]::ReadAllText((Join-Path $BuildDirectory 'LIBRARY.txt'))
    foreach ($g in 1, 2) {
        if (([regex]::Matches($library, "(?m)^PASS generation $g cumulative library ")).Count -ne 11) {
            throw "Library suite count mismatch: $g"
        }
    }
    if (([regex]::Matches($library, '(?m)^PASS identical library ')).Count -ne 22) {
        throw 'Library artifact pair count mismatch'
    }
    if (!$library.TrimEnd().EndsWith('PASS both generations cumulative library suites')) {
        throw 'Missing library final gate'
    }
    'PASS 11 cumulative library suites per generation and 22 library artifact pairs'
    & (Join-Path $PSScriptRoot 'tools/audit-feature-artifacts.ps1') -BuildDirectory $BuildDirectory
    if (!$?) {
        throw 'Feature artifact byte audit failed'
    }
    if (-not $log.TrimEnd().EndsWith('PASS complete seed-to-object-toolchain build and final regressions')) {
        throw 'Missing seed build final gate'
    }
    & (Join-Path $PSScriptRoot 'base/audit.ps1') -BuildDirectory (Join-Path $BuildDirectory 'base') -VerificationLog (Join-Path $BuildDirectory 'BASE-VERIFICATION.txt')
    if (-not $?) {
        throw 'Base audit failed'
    }
    foreach ($g in 1, 2) {
        $reg = [IO.File]::ReadAllText((Join-Path $BuildDirectory ('REGRESSION-' + $g + '.txt')))
        if (-not $reg.TrimEnd().EndsWith('4992 C1 tests passed.') -or ([regex]::Matches($reg, '(?m)^PASS C1 ')).Count -ne 4992) {
            throw "Final regression $g incomplete"
        }
    }
    'PASS final compilers: 4992 checks per generation'
}
if (!$Smoke) {
    & (Join-Path $PSScriptRoot 'diagnostics/audit.ps1') -BuildDirectory $BuildDirectory
    if (!$?) {
        throw 'Diagnostic artifact audit failed'
    }
}
Get-FileHash (Join-Path $BuildDirectory 'host/c1-stage1.exe')
Get-FileHash (Join-Path $BuildDirectory 'host/c1-stage2.exe')

if (!$Smoke) {
    & (Join-Path $PSScriptRoot 'namespace/audit-constant-comma.ps1') -BuildDirectory $BuildDirectory
    if (!$?) {
        throw 'Constant comma audit failed'
    }
}
if (!$Smoke) {
    & (Join-Path $PSScriptRoot 'namespace/audit-empty-unit.ps1') -BuildDirectory $BuildDirectory
    if (!$?) {
        throw 'Empty translation unit audit failed'
    }
}
if (!$Smoke) {
    & (Join-Path $PSScriptRoot 'namespace/audit-vla.ps1') -BuildDirectory $BuildDirectory
    if (!$?) {
        throw 'VLA audit failed'
    }
}
if (!$Smoke) {
    & (Join-Path $PSScriptRoot 'complex/audit.ps1') -BuildDirectory $BuildDirectory
    if (!$?) {
        throw 'Complex/current runtime audit failed'
    }
}
