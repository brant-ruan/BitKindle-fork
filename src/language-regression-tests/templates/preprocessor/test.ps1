param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath($Snapshot)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$source = Get-Content (Join-Path $snapshot 'base/tests/test-c0.ps1') -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'"))
$prefix = $prefix.Replace("'^C1 error: '", "'(?m)^C1 error: '")
$cases = @(
    @{name = 'translation-wide-signed'; expr = '2147483647 + 1 == 2147483648' },
    @{name = 'translation-wide-unsigned'; expr = '0xffffffffU + 1 == 0x100000000U' },
    @{name = 'translation-signed-hex'; expr = '0xffffffff > -1' },
    @{name = 'translation-unsigned-wrap'; expr = '0xffffffffffffffffUL + 1 == 0' },
    @{name = 'unsigned-negative-compare'; expr = '(-1 < 1U) == 0' },
    @{name = 'unsigned-subtraction'; expr = '(1U-2) > 0' },
    @{name = 'conditional-common-type'; expr = '((1 ? -1 : 1U) < 0) == 0' },
    @{name = 'conditional-short-division'; expr = '(1 ? 7 : 1/0) == 7' },
    @{name = 'conditional-short-other'; expr = '(0 ? 1/0 : 7) == 7' },
    @{name = 'and-short-division'; expr = '(0 && 1/0) == 0' },
    @{name = 'or-short-division'; expr = '1 || 1/0' },
    @{name = 'and-short-shift'; expr = '(0 && (1 << -1)) == 0' },
    @{name = 'conditional-short-shift'; expr = '(1 ? 7 : (1 << -1)) == 7' },
    @{name = 'comparison-result-signed'; expr = '(((2U>1U)-2) < 0)' },
    @{name = 'logical-result-signed'; expr = '(((2U&&1U)-2) < 0)' },
    @{name = 'not-result-signed'; expr = '((!0U)-2) < 0' },
    @{name = 'remaining-keyword-zero'; expr = 'int == 0 && undefined_name == 0' },
    @{name = 'character-escapes'; expr = "'\101' == 'A' && '\x41' == 'A'" },
    @{name = 'precedence'; expr = '(1 | 2 & 4) == 1 && (1 + 2 << 2) == 12' },
    @{name = 'conditional-associativity'; expr = '(0 ? 1 : 0 ? 2 : 3) == 3' },
    @{name = 'reject-unevaluated-float'; expr = '1 || 1.0'; reject = $true },
    @{name = 'reject-unevaluated-cast'; expr = '1 || (int)1'; reject = $true },
    @{name = 'reject-unevaluated-bad-number'; expr = '1 || 09'; reject = $true },
    @{name = 'reject-active-shift'; expr = '1 << -1'; reject = $true },
    @{name = 'reject-active-mod-zero'; expr = '1 % 0'; reject = $true }
)
$commands = foreach ($c in $cases) {
    $body = "#if $($c.expr)`nint main(void){return 0;}`n#else`n#error unexpected_condition`n#endif`n"
    "Check '$($c.name)' '" + $body.Replace("'", "''") + "' 0 " + $(if ($c.reject) {
            '$true'
        }
        else {
            '$false'
        })
}
$selected = $prefix + ($commands -join "`n") + "`n" + '"PASS $script:passed preprocessing expression cases"'
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'selected.ps1'), $selected)
$cases | ConvertTo-Json | Set-Content (Join-Path $PSScriptRoot 'CASES.json')
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($mode in 'c89', 'c99') {
        foreach ($g in 1, 2) {
            $env:C1_STANDARD = $mode
            & ([scriptblock]::Create($selected)) -Compiler "$build/host/c1-stage$g.exe" -Assembler "$build/link-stage2.exe" -CompilerKind C1 -ScratchDirectory (Join-Path $PSScriptRoot "$mode-$g")
            if (!$?) {
                throw 'Preprocessor expression suite failed'
            }
        }
    }
    "PASS $($cases.Count) preprocessing expression cases in each mode and generation"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
