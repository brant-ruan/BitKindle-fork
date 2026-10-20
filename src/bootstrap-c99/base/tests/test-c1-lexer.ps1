param(
    [Parameter(Mandatory = $true)][string]$Lexer,
    [string]$ReferenceTokens = '',
    [string]$ScratchDirectory = (Join-Path $PSScriptRoot 'scratch-lexer')
)
$ErrorActionPreference = 'Stop'
$Lexer = [IO.Path]::GetFullPath($Lexer)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
New-Item -ItemType Directory -Force -Path $ScratchDirectory | Out-Null
$script:passed = 0
function Check([string]$Name, [string]$Source, [string]$Expected, [int]$Status = 0) {
    $inputPath = Join-Path $ScratchDirectory "$Name.c"
    $outputPath = Join-Path $ScratchDirectory "$Name.tokens"
    [IO.File]::WriteAllText($inputPath, $Source, [Text.Encoding]::ASCII)
    $p = Start-Process -FilePath $Lexer -WindowStyle Hidden -PassThru -RedirectStandardInput $inputPath -RedirectStandardOutput $outputPath
    [void]$p.Handle
    $timeout = 10000
    if ($Name -eq 'own-source') {
        $timeout = 60000 
    }
    if (-not $p.WaitForExit($timeout)) {
        $p.Kill()
        throw "$Name lexer timeout" 
    }
    $p.WaitForExit()
    if ($p.ExitCode -ne $Status) {
        throw "$Name lexer exit $($p.ExitCode), expected $Status" 
    }
    $actual = [IO.File]::ReadAllText($outputPath)
    if ($Expected -ne '*unchecked*' -and $actual -cne $Expected) {
        throw "$Name tokens differ: $actual" 
    }
    $script:passed++
    "PASS C1 lexer $Name"
}
Check 'keywords' 'int main(void){return 0x2A;}' "256:int`n262:main`n40`n261:void`n41`n123`n257:return`n144:42`n59`n125`n"
Check 'names' 'alpha alpha beta' "268:alpha`n268:alpha`n269:beta`n"
Check 'operators' '== != <= >= << >> && || + -' "128`n129`n130`n131`n132`n133`n134`n135`n43`n45`n"
Check 'numbers' '0 010 0x2A 2147483647' "144:0`n144:8`n144:42`n144:2147483647`n"
Check 'comments' "/* comment */ int // line`nreturn" "256:int`n257:return`n"
Check 'characters' "'A' '\n' '\0'" "144:65`n144:10`n144:0`n"
Check 'empty' '' ''
Check 'bad-octal' '08' '' 1
Check 'empty-hex' '0x' '' 1
Check 'overflow' '2147483648' '' 1
Check 'bad-suffix' '12xyz' '' 1
Check 'bad-comment' '/* unterminated' '' 1
Check 'bad-character' "'ab'" '' 1
Check 'long-name' ('a' * 64) '' 1
$ownExpected = '*unchecked*'
if ($ReferenceTokens) {
    $ownExpected = [IO.File]::ReadAllText([IO.Path]::GetFullPath($ReferenceTokens)) 
}
Check 'own-source' (Get-Content (Join-Path $PSScriptRoot '../c1/lexer.c') -Raw) $ownExpected
"$script:passed C1 lexer tests passed."
