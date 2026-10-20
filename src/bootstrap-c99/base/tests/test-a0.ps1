param(
    [Parameter(Mandatory = $true)][string]$Assembler,
    [string]$ScratchDirectory = (Join-Path $PSScriptRoot 'scratch')
)
$ErrorActionPreference = 'Stop'
$Assembler = [IO.Path]::GetFullPath($Assembler)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
New-Item -ItemType Directory -Force -Path $ScratchDirectory | Out-Null
$script:passed = 0

function Check([string]$Name, [string]$Source, [string]$Expected, [int]$ExitCode = 0) {
    $inputPath = Join-Path $ScratchDirectory "$Name.a0"
    $outputPath = Join-Path $ScratchDirectory "$Name.bin"
    # This is test input text, never an externally produced executable.
    [IO.File]::WriteAllText($inputPath, $Source, [Text.Encoding]::ASCII)
    $process = Start-Process -FilePath $Assembler -WindowStyle Hidden -PassThru `
        -RedirectStandardInput $inputPath -RedirectStandardOutput $outputPath
    [void]$process.Handle
    if (-not $process.WaitForExit(10000)) {
        $process.Kill()
        throw "$Name timed out"
    }
    $process.WaitForExit()
    if ($process.ExitCode -ne $ExitCode) {
        throw "$Name exit $($process.ExitCode), expected $ExitCode" 
    }
    $bytes = [IO.File]::ReadAllBytes($outputPath)
    if ($Expected.StartsWith('length:')) {
        $length = [int]$Expected.Substring(7)
        if ($bytes.Length -ne $length) {
            throw "$Name length $($bytes.Length), expected $length" 
        }
        foreach ($b in $bytes) {
            if ($b -ne 0) {
                throw "$Name expected only zero bytes" 
            } 
        }
    }
    else {
        $actual = [BitConverter]::ToString($bytes).Replace('-', '')
        if ($actual -cne $Expected.Replace(' ', '').ToUpperInvariant()) {
            throw "$Name bytes $actual, expected $Expected" 
        }
    }
    $script:passed++
    "PASS $Name"
}

Check 'empty' '' ''
Check 'literal' '00 01 7f 80 aA Bb cC dD eE fF' '00017F80AABBCCDDEEFF'
Check 'adjacent' '0123456789abcdef' '0123456789ABCDEF'
Check 'whitespace' " `t`r`n00`r`nFF" '00FF'
Check 'comments' "; comment`r`n12; tail without newline" '12'
Check 'forward' '@0001 :0001' '04000000'
Check 'backward' ':0001 90 %0001' '90FBFFFFFF'
Check 'smoke' (Get-Content (Join-Path $PSScriptRoot 'smoke.a0') -Raw) 'AABB12100000080000000010000000000000CCE9FFFFFF'
Check 'zero-label' ':0000 @0000 &0000' '000000000000000000000000'
Check 'last-label' ':FFFF @ffff' '00000000'
Check 'origin-no-padding' '^12345678 :0001 @0001' '78563412'
Check 'rel-positive-limit' '%0001 ^80000003 :0001' 'FFFFFF7F'
Check 'rel-negative-limit' '^80000000 %0001 ^00000004 :0001' '00000080'
Check 'abs32-limit' '^FFFFFFFF :0001 @0001' 'FFFFFFFF'
Check 'abs64-cross-4gb' '^FFFFFFFF 00 :0001 ^00000000 &0001' '000000000001000000'
Check 'undefined' '@0001' '' 1
Check 'duplicate' ':0001 :0001' '' 1
Check 'bad-token' '?' '' 1
Check 'bad-hex' '0G' '' 1
Check 'short-byte' 'F' '' 1
Check 'short-label' ':012' '' 1
Check 'short-reference' '%01' '' 1
Check 'short-origin' '^0123' '' 1
Check 'rel-positive-overflow' '%0001 ^80000004 :0001' '' 1
Check 'rel-negative-overflow' '^80000001 %0001 ^00000004 :0001' '' 1
Check 'abs32-overflow' '^FFFFFFFF 00 :0001 ^00000000 @0001' '' 1
Check 'late-error-no-output' 'AA BB CC @1234' '' 1
Check 'exact-output-capacity' (':0000 ' + ('&0000' * 131072)) 'length:1048576'
Check 'output-overflow' (':0000 ' + ('&0000' * 131073)) '' 1
Check 'input-capacity-rejected' (' ' * 1048576) '' 1
"$script:passed assembler tests passed."
