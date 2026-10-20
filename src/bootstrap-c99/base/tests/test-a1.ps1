param(
    [Parameter(Mandatory = $true)][string]$Assembler,
    [string]$ScratchDirectory = (Join-Path $PSScriptRoot 'scratch-a1')
)
$ErrorActionPreference = 'Stop'
$Assembler = [IO.Path]::GetFullPath($Assembler)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
New-Item -ItemType Directory -Force -Path $ScratchDirectory | Out-Null
$script:passed = 0
function Check([string]$Name, [string]$Source, [string]$Expected, [int]$ExitCode = 0) {
    $inputPath = Join-Path $ScratchDirectory "$Name.a1"
    $outputPath = Join-Path $ScratchDirectory "$Name.bin"
    [IO.File]::WriteAllText($inputPath, $Source, [Text.Encoding]::ASCII)
    $p = Start-Process -FilePath $Assembler -WindowStyle Hidden -PassThru -RedirectStandardInput $inputPath -RedirectStandardOutput $outputPath
    [void]$p.Handle
    if (-not $p.WaitForExit(10000)) {
        $p.Kill()
        throw "$Name timeout" 
    }
    $p.WaitForExit()
    if ($p.ExitCode -ne $ExitCode) {
        throw "$Name exit $($p.ExitCode), expected $ExitCode" 
    }
    $actual = [BitConverter]::ToString([IO.File]::ReadAllBytes($outputPath)).Replace('-', '')
    if ($actual -cne $Expected.Replace(' ', '').ToUpperInvariant()) {
        throw "$Name bytes $actual, expected $Expected" 
    }
    $script:passed++
    "PASS $Name"
}
Check 'named-forward' '@finish AA :finish' '05000000AA'
Check 'named-backward' ':start AA %start' 'AAFBFFFFFF'
Check 'named-wide' '^12345678 :address &address' '7856341200000000'
Check 'distinct-prefixes' ':a 01 :ab 02 @a @ab' '01020000000001000000'
Check 'case-sensitive' ':name @Name' '' 1
Check 'name-63' (':' + ('n' * 63) + ' @' + ('n' * 63)) '00000000'
Check 'name-64' (':' + ('n' * 64)) '' 1
Check 'empty-name' ': ' '' 1
Check 'duplicate-name' ':same :same' '' 1
Check 'raw-string' '"Hello; labels : @ %"' '48656C6C6F3B206C6162656C73203A20402025'
Check 'empty-string' '"" AA' 'AA'
Check 'unterminated-string' '"missing' '' 1
Check 'zero-fill' 'AA ~00000003 BB ~00000000' 'AA000000BB'
Check 'zero-overflow' '~00100001' '' 1
Check 'all-mnemonics' '!ret !nop !int3 !push_rax !pop_rcx !add !sub !mul !neg !mov_eax !mov_rax !call !jmp !jz !jnz !enter !lea_rax !load_rax !store_rax !cmp !xor' 'C390CC50594801C84829C8480FAFC148F7D8B848B8E8E90F840F854881EC488D05488B004889014839C84831C8'
Check 'mnemonic-operand' '!mov_eax 2A000000 !ret' 'B82A000000C3'
Check 'mnemonic-relocation' '!jmp %end 90 :end !ret' 'E90100000090C3'
Check 'unknown-mnemonic' '!missing' '' 1
Check 'symbol-mnemonic-same-name' ':ret !ret @ret' 'C300000000'

$exe = Join-Path $ScratchDirectory 'factorial.exe'
$p = Start-Process -FilePath $Assembler -WindowStyle Hidden -PassThru -RedirectStandardInput (Join-Path $PSScriptRoot 'factorial.a1') -RedirectStandardOutput $exe
[void]$p.Handle
if (-not $p.WaitForExit(10000)) {
    $p.Kill()
    throw 'factorial assembly timeout' 
}
$p.WaitForExit()
if ($p.ExitCode -ne 0) {
    throw 'factorial assembly failed' 
}
$p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
[void]$p.Handle
if (-not $p.WaitForExit(10000)) {
    $p.Kill()
    throw 'factorial execution timeout' 
}
$p.WaitForExit()
if ($p.ExitCode -ne 120) {
    throw "factorial returned $($p.ExitCode), expected 120" 
}
$script:passed++
"PASS factorial.exe returned 120"
"$script:passed A1 tests passed."
