param([int]$Generation = 1, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../tools/compile.ps1')
$root = Join-Path $BuildDirectory ('integration/imports' + $Generation)
New-Item -ItemType Directory -Force $root | Out-Null
$compiler = Join-Path $BuildDirectory ('host/c1-stage' + $Generation + '.exe')
$linker = Join-Path $BuildDirectory ('link-stage' + $Generation + '.exe')
function Run([string]$Exe, [string]$Arguments, [int]$Expected, [string]$Label) {
    $p = Start-Process -FilePath $Exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root ($Label + '.stdout')) -RedirectStandardError (Join-Path $root ($Label + '.stderr'))
    [void]$p.Handle
    if (-not $p.WaitForExit(30000)) {
        $p.Kill()
        throw "Timeout $Label"
    }
    $p.WaitForExit()
    if ($p.ExitCode -ne $Expected) {
        throw "$Label returned $($p.ExitCode), expected $Expected"
    }
}
$cases = @(
    @('pid', 'unsigned long pid(void);int main(void){return pid()?42:1;}', '--import pid KERNEL32.dll GetCurrentProcessId', 42),
    @('two-dlls', 'unsigned long pid(void);unsigned long upper(char *,unsigned long);int main(void){char text[]="abc";if(upper(text,3)!=3||text[0]!=65||text[1]!=66||text[2]!=67)return 1;return pid()?42:2;}', '--import pid KERNEL32.dll GetCurrentProcessId --import upper USER32.dll CharUpperBuffA', 42),
    @('import-pointer', 'unsigned long pid(void);unsigned long (*pointer)(void)=pid;int main(void){return pointer()?42:1;}', '--import pid KERNEL32.dll GetCurrentProcessId', 42),
    @('exit-process', 'void finish(unsigned int);int main(void){finish(37);return 99;}', '--import finish KERNEL32.dll ExitProcess', 37)
)
$old = $env:C1_OUTPUT
try {
    foreach ($case in $cases) {
        $name = $case[0]
        $source = Join-Path $root ($name + '.c')
        $obj = Join-Path $root ($name + '.c1o')
        $exe = Join-Path $root ($name + '.exe')
        [IO.File]::WriteAllText($source, $case[1], [Text.Encoding]::ASCII)
        $env:C1_OUTPUT = 'object'
        Compile $compiler $source $obj
        $env:C1_OUTPUT = $null
        Run $linker ('"' + $exe + '" main "' + $obj + '" ' + $case[2]) 0 ($name + '-link')
        Run $exe ' ' $case[3] $name
        "PASS imported C program $name"
    }
    $destination = Join-Path $root 'pid.exe'
    $obj = Join-Path $root 'pid.c1o'
    $before = [IO.File]::ReadAllBytes($destination)
    $badOptions = @('--import pid KERNEL32.dll #65536', '--import pid KERNEL32.dll', '--import pid KERNEL32.dll GetCurrentProcessId --import pid KERNEL32.dll GetCurrentProcessId')
    for ($i = 0; $i -lt $badOptions.Count; $i++) {
        Run $linker ('"' + $destination + '" main "' + $obj + '" ' + $badOptions[$i]) 1 ('rejected-' + $i)
        $after = [IO.File]::ReadAllBytes($destination)
        if ($before.Length -ne $after.Length) {
            throw 'Output truncated'
        }
        for ($j = 0; $j -lt $before.Length; $j++) {
            if ($before[$j] -ne $after[$j]) {
                throw 'Output modified'
            }
        }
        "PASS invalid import options $i preserve output"
    }
}
finally {
    $env:C1_OUTPUT = $old
}
