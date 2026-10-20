param([Parameter(Mandatory = $true)][string]$Compiler, [Parameter(Mandatory = $true)][string]$Assembler, [string]$ScratchDirectory = (Join-Path $PSScriptRoot 'full-scratch'))
$ErrorActionPreference = 'Stop'
$BaselineRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../base/tests'))
$original = Get-Content (Join-Path $BaselineRoot 'test-c0.ps1') -Raw
# The baseline compiler retains its historical 63-character rejection; the final
# compiler accepts full spellings with 63 significant characters.
$old = "Check 'long-name' ('int main(void){int '+('a'*64)+';return 0;}') 0 `$true"
$new = "Check 'long-name' ('int main(void){int '+('a'*64)+';return 0;}') 0"
if (!$original.Contains($old)) {
    throw 'Long-name fixture changed'
}
$adapted = $original.Replace($old, $new).Replace('$PSScriptRoot', '$BaselineRoot')
# The final compiler implements automatic VLAs. Keep invalid constant bounds
# in file scope, and execute the formerly unsupported local-array case.
$boundChanges = @(
    @("Check 'bound-overflow-expression' 'int main(void){int a[2147483647+1];return 0;}' 0 `$true", "Check 'bound-overflow-expression' 'int a[2147483647+1];int main(void){return 0;}' 0 `$true"),
    @("Check 'bound-runtime-unsupported' 'int main(void){int n=2;int a[n];return 0;}' 0 `$true", "Check 'bound-runtime-supported' 'int main(void){int n=2;int a[n];a[1]=7;return sizeof a+a[1];}' 15"),
    @("Check 'bound-comma-rejected' 'int main(void){int a[(1,2)];return 0;}' 0 `$true", "Check 'bound-comma-rejected' 'int a[(1,2)];int main(void){return 0;}' 0 `$true")
)
foreach ($change in $boundChanges) {
    if (($adapted.Split(@($change[0]), [StringSplitOptions]::None)).Count -ne 2) {
        throw 'VLA baseline fixture changed'
    }
    $adapted = $adapted.Replace($change[0], $change[1])
}
# The final compiler may issue extension warnings before its required error.
# Keep the rejection and zero-output gates; recognize a complete error line.
$errorAnchor = "-notmatch '^C1 error: '"
if (!$adapted.Contains($errorAnchor)) {
    throw 'Baseline error diagnostic check changed'
}
$adapted = $adapted.Replace($errorAnchor, "-notmatch '(?m)^C1 error: '")
$C89DiagnosticCases = @(
    'npc-c89-unselected-comma', 'npc-c89-unparenthesized-middle-comma',
    'vr-c89-all-three', 'vr-c89-repeat-qualifiers',
    'vr-c89-restrict-local', 'vr-c89-restrict-parameters', 'vr-c89-restrict-typedef',
    'vr-c89-restrict-array-typedef', 'vr-c89-restrict-void', 'vr-c89-restrict-incomplete',
    'vr-c89-restrict-function-pointer-object', 'vr-c89-restrict-qualified-pointer',
    'vr-c89-restrict-nested-scope', 'vr-c89-restrict-list'
    'const-c89-repeated', 'const-c89-typedef', 'const-c89-record-const-pointer-member'
)
$script:modeAdjustedCases = 0
$anchor = '    $c = Join-Path $ScratchDirectory "$Name.c"'
if (($adapted.Split(@($anchor), [StringSplitOptions]::None)).Count -ne 2) {
    throw 'Check hook must be unique'
}
$hook = @'
    if($Name -in $C89DiagnosticCases){
        if($Reject){throw "C89 case already requires rejection: $Name"}
        if($Name -like 'npc-c89-*'){
            if(!$Source.Contains(',')){throw 'Comma fixture changed'}
        }elseif($Name -eq 'const-c89-repeated'){
            if(!$Source.Contains('const const T')){throw 'Repeated const fixture changed'}
        }elseif($Name -eq 'const-c89-typedef'){
            if(!$Source.Contains('const CC q')){throw 'Const typedef fixture changed'}
        }elseif($Name -eq 'const-c89-record-const-pointer-member'){
            if(!$Source.Contains('struct S s={&n}')){throw 'Aggregate address fixture changed'}
        }elseif($Name -eq 'vr-c89-repeat-qualifiers'){
            if(!$Source.Contains('volatile volatile const const')){throw 'Repeated qualifier fixture changed'}
        }elseif($Source -notmatch '\brestrict\b'){throw "Restrict fixture changed: $Name"}
        $Reject=$true
        $script:modeAdjustedCases++
    }
'@
$adapted = $adapted.Replace($anchor, $hook + "`n" + $anchor)
# The selected current compiler first bootstraps the historical compiler.
# Compare two subsequent builds made by that historical compiler itself.
$integerModes = Get-Content (Join-Path $BaselineRoot 'test-integer-modes.ps1') -Raw
$oldRead = '    $one=[IO.File]::ReadAllBytes($generation1);$two=[IO.File]::ReadAllBytes($generation2)'
$newRead = @"
    `$generation3=Join-Path `$ScratchDirectory 'integer-c99-self3.exe'
    if((Native `$generation2 `$own `$generation3 `$selfError) -ne 0){throw 'C99 third legacy compilation failed'}
    `$one=[IO.File]::ReadAllBytes(`$generation2);`$two=[IO.File]::ReadAllBytes(`$generation3)
"@
if (($integerModes.Split(@($oldRead), [StringSplitOptions]::None)).Count -ne 2) {
    throw 'Legacy generation anchor changed'
}
$integerModes = $integerModes.Replace($oldRead, $newRead).Replace('$PSScriptRoot', '$BaselineRoot')
$oldInclude = ". (Join-Path `$BaselineRoot 'test-integer-modes.ps1')"
if (($adapted.Split(@($oldInclude), [StringSplitOptions]::None)).Count -ne 2) {
    throw 'Integer mode include anchor changed'
}
$adapted = $adapted.Replace($oldInclude, '. ([scriptblock]::Create($integerModes))')
& ([scriptblock]::Create($adapted)) -Compiler $Compiler -Assembler $Assembler -CompilerKind C1 -ScratchDirectory $ScratchDirectory
if (!$?) {
    throw 'Final compiler regression failed'
}
if ($script:modeAdjustedCases -ne 17) {
    throw "Expected 17 C89 diagnostic cases, found $script:modeAdjustedCases"
}
