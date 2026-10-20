param(
    [Parameter(Mandatory = $true)][string]$Compiler,
    [Parameter(Mandatory = $true)][string]$Assembler,
    [ValidateSet('C0', 'C1')][string]$CompilerKind = 'C0',
    [switch]$CapacityBridgeOnly,
    [string]$ScratchDirectory = (Join-Path $PSScriptRoot 'scratch-c0')
)
$ErrorActionPreference = 'Stop'
$Compiler = [IO.Path]::GetFullPath($Compiler)
$Assembler = [IO.Path]::GetFullPath($Assembler)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
New-Item -ItemType Directory -Force -Path $ScratchDirectory | Out-Null
$script:passed = 0
function Native([string]$Exe, [string]$InputFile, [string]$OutputFile, [string]$ErrorFile) {
    if ($CompilerKind -eq 'C0') {
        $parameters = @{FilePath = $Exe
            WindowStyle = 'Hidden'
            PassThru = $true
        }
        if ($InputFile) {
            $parameters.RedirectStandardInput = $InputFile 
        }
        if ($OutputFile) {
            $parameters.RedirectStandardOutput = $OutputFile 
        }
        if ($ErrorFile) {
            $parameters.RedirectStandardError = $ErrorFile 
        }
        $p = Start-Process @parameters
        [void]$p.Handle
        if (-not $p.WaitForExit(10000)) {
            $p.Kill()
            throw "Timeout: $Exe" 
        }
        $p.WaitForExit()
        return $p.ExitCode
    }
    # Byte-preserving standard-stream transport only; compiler output is never transformed.
    $info = [Diagnostics.ProcessStartInfo]::new($Exe)
    $info.UseShellExecute = $false
    if ($Exe -ne $Compiler) {
        $info.Arguments = $script:TestArguments
    }
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $p = [Diagnostics.Process]::new()
    $p.StartInfo = $info
    $outputStream = $null
    $errorStream = $null
    $inputStream = $null
    try {
        $outputStream = if ($OutputFile) {
            [IO.File]::Create($OutputFile)
        }
        else {
            [IO.MemoryStream]::new()
        }
        $errorStream = if ($ErrorFile) {
            [IO.File]::Create($ErrorFile)
        }
        else {
            [IO.MemoryStream]::new()
        }
        [void]$p.Start()
        $outTask = $p.StandardOutput.BaseStream.CopyToAsync($outputStream)
        $errTask = $p.StandardError.BaseStream.CopyToAsync($errorStream)
        if ($InputFile) {
            $inputStream = [IO.File]::OpenRead($InputFile)
            $inTask = $inputStream.CopyToAsync($p.StandardInput.BaseStream)
            if (-not $inTask.Wait(10000)) {
                throw "Input timeout: $Exe"
            }
        }
        $p.StandardInput.Close()
        if (-not $p.WaitForExit(10000)) {
            throw "Timeout: $Exe"
        }
        if (-not [Threading.Tasks.Task]::WaitAll([Threading.Tasks.Task[]]@($outTask, $errTask), 10000)) {
            throw "Output timeout: $Exe"
        }
        return $p.ExitCode
    }
    finally {
        if ($p.Id -and -not $p.HasExited) {
            $p.Kill()
        }
        if ($inputStream) {
            $inputStream.Dispose()
        }
        if ($outputStream) {
            $outputStream.Dispose()
        }
        if ($errorStream) {
            $errorStream.Dispose()
        }
        $p.Dispose()
    }
}
function Check([string]$Name, [string]$Source, [int]$Result = 0, [bool]$Reject = $false, [string]$RuntimeInput = '', [string]$ExpectedOutput = '', [string]$ExpectedError = '') {
    $c = Join-Path $ScratchDirectory "$Name.c"
    $a = Join-Path $ScratchDirectory "$Name.a1"
    $e = Join-Path $ScratchDirectory "$Name.err"
    $exe = Join-Path $ScratchDirectory "$Name.exe"
    if ($CompilerKind -eq 'C1') {
        $a = $exe 
    }
    [IO.File]::WriteAllText($c, $Source, [Text.Encoding]::ASCII)
    $status = Native $Compiler $c $a $e
    if ($Reject) {
        if ($status -ne 1) {
            throw "$Name must be rejected, exit $status" 
        }
        if ((Get-Item -LiteralPath $a).Length -ne 0) {
            throw "$Name published partial assembly" 
        }
        if ($CompilerKind -eq 'C0' -and (Get-Content -LiteralPath $e -Raw) -notmatch 'C0 error near input byte 0x[0-9A-F]{8}') {
            throw "$Name missing diagnostic" 
        }
    }
    else {
        if ($status -ne 0) {
            throw "$Name compile failed: $(Get-Content -LiteralPath $e -Raw)" 
        }
        if ($CompilerKind -eq 'C0' -and (Native $Assembler $a $exe '') -ne 0) {
            throw "$Name assembly failed" 
        }
        $pe = [IO.File]::ReadAllBytes($exe)
        if ($pe.Length -lt 1024 -or $pe.Length % 512 -ne 0 -or $pe[0] -ne 77 -or $pe[1] -ne 90 -or [BitConverter]::ToUInt32($pe, 0x198) + 512 -ne $pe.Length) {
            throw "$Name incorrect PE padding/code-size accounting" 
        }
        if ($CompilerKind -eq 'C0' -and $pe.Length -ne 262656) {
            throw "$Name incorrect C0 output size" 
        }
        if ($CompilerKind -eq 'C1') {
            $textEnd = [BitConverter]::ToUInt32($pe, 0x194) + [Math]::Max([BitConverter]::ToUInt32($pe, 0x190), [BitConverter]::ToUInt32($pe, 0x198))
            $bssStart = [BitConverter]::ToUInt32($pe, 0x1bc)
            $bssEnd = $bssStart + [BitConverter]::ToUInt32($pe, 0x1b8)
            if ($textEnd -gt $bssStart -or $bssEnd -ne [BitConverter]::ToUInt32($pe, 0xd0)) {
                throw "$Name overlapping PE sections or incorrect image span" 
            }
        }
        $runInput = Join-Path $ScratchDirectory "$Name.stdin"
        $runOutput = Join-Path $ScratchDirectory "$Name.stdout"
        [IO.File]::WriteAllText($runInput, $RuntimeInput, [Text.Encoding]::ASCII)
        $runError = Join-Path $ScratchDirectory "$Name.stderr"
        $status = Native $exe $runInput $runOutput $runError
        if ([IO.File]::ReadAllText($runError) -cne $ExpectedError) {
            throw "$Name unexpected stderr: $([IO.File]::ReadAllText($runError))"
        }
        if ($status -ne $Result) {
            throw "$Name returned $status, expected $Result" 
        }
        if ([IO.File]::ReadAllText($runOutput) -cne $ExpectedOutput) {
            throw "$Name unexpected program stdout" 
        }
    }
    $script:passed++
    "PASS $CompilerKind $Name"
}

if ($CapacityBridgeOnly) {
    if ($CompilerKind -ne 'C1') {
        throw 'Capacity bridge requires the C1 PE transport' 
    }
    $bridgeMain = 'int main(void){return 7;}'
    Check 'bridge-source-limit' ($bridgeMain + (' ' * (524288 - $bridgeMain.Length))) 7
    Check 'bridge-source-over-limit' ($bridgeMain + (' ' * (524289 - $bridgeMain.Length))) 0 $true
    Check 'bridge-code-over-256k' ('int main(void){' + ('1;' * 100000) + 'return 9;}') 9
    if ((Get-Item -LiteralPath (Join-Path $ScratchDirectory 'bridge-code-over-256k.exe')).Length -le 262656) {
        throw 'Bridge code test did not cross the old output limit'
    }
    Check 'bridge-code-over-limit' ('int main(void){' + ('1;' * 120000) + 'return 9;}') 0 $true
    "$script:passed capacity bridge tests passed."
    return
}

function PpCheck([string]$Name, [string[]]$Lines, [int]$Result = 0, [bool]$Reject = $false) {
    Check $Name ([string]::Join([string][char]10, $Lines)) $Result $Reject
}
function PpString([string]$Name, [string[]]$Lines, [string]$Expression, [string]$Expected) {
    $program = 'int main(void){char *p=' + $Expression + ';while(*p){__c0_putchar(*p);p++;}return 0;}'
    Check $Name ([string]::Join([string][char]10, ($Lines + @($program)))) 0 $false '' $Expected
}


. (Join-Path $PSScriptRoot 'tests.ps1')
. (Join-Path $PSScriptRoot '../time-checks/tests-all.ps1')
foreach ($feature in 'remove-directory', 'system-command', 'wide-literals') {
    . (Join-Path $PSScriptRoot ('../feature-checks/' + $feature + '.ps1'))
}
"$script:passed combined feature checks passed."