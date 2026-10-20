param([int]$Generation = 1, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../tools/compile.ps1')
$root = Join-Path $BuildDirectory ('integration/objects' + $Generation)
New-Item -ItemType Directory -Force $root | Out-Null
$tool = Join-Path $BuildDirectory ('host/c1-stage' + $Generation + '.exe')
$linker = Join-Path $BuildDirectory ('link-stage' + $Generation + '.exe')
function Run([string]$Exe, [string]$Arguments, [int]$Expected) {
    $p = Start-Process -FilePath $Exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    [void]$p.Handle
    if (-not $p.WaitForExit(10000)) {
        $p.Kill()
        throw 'Run timeout'
    }
    $p.WaitForExit()
    if ($p.ExitCode -ne $Expected) {
        throw "Expected $Expected, got $($p.ExitCode): $Exe"
    }
}
$cases = @(
    @('call', 'extern int answer(int);int main(void){return answer(20)+2;}', 'int answer(int n){return n*2;}', 42),
    @('common', 'extern int storage;extern int read_value(void);int main(void){storage=37;return read_value();}', 'int storage;int read_value(void){return storage;}', 37),
    @('pointer', 'extern int answer(int);int main(void){int (*p)(int)=answer;return p(21);}', 'int answer(int n){return n*2;}', 42),
    @('string', 'extern int answer(void);int main(void){return answer();}', 'static int local(void){return "*"[0];}int answer(void){return local();}', 42),
    @('static', 'extern int answer(void);static int private_value;int main(void){private_value=2;return answer()+private_value;}', 'static int private_value;int answer(void){private_value=40;return private_value;}', 42),
    @('initialized', 'extern int value;int main(void){return value;}', 'int value=42;', 42),
    @('both-initializers', 'extern int answer(void);int value=2;int main(void){return answer()+value;}', 'static int value=40;int answer(void){return value;}', 42),
    @('address-initializer', 'extern int value;int *pointer=&value;int main(void){return *pointer;}', 'int value=42;', 42),
    @('array-initializer', 'extern int answer(void);int main(void){return answer();}', 'int values[]={20,22};int answer(void){return values[0]+values[1];}', 42),
    @('local-static-initializer', 'extern int answer(void);int main(void){return answer()+answer();}', 'int answer(void){static int count=19;return ++count;}', 41),
    @('strong-over-common', 'int value;int main(void){return value;}', 'int value=42;', 42)
)
$old = $env:C1_OUTPUT
try {
    foreach ($case in $cases) {
        $name = $case[0]
        $objects = @()
        for ($i = 0; $i -lt 2; $i++) {
            $source = Join-Path $root ($name + '-' + $i + '.c')
            $object = Join-Path $root ($name + '-' + $i + '.c1o')
            [IO.File]::WriteAllText($source, $case[$i + 1], [Text.Encoding]::ASCII)
            $env:C1_OUTPUT = 'object'
            Compile $tool $source $object
            $objects += $object
        }
        $env:C1_OUTPUT = $null
        $exe = Join-Path $root ($name + '.exe')
        Run $linker ('"' + $exe + '" main "' + $objects[0] + '" "' + $objects[1] + '"') 0
        Run $exe ' ' $case[3]
        "PASS compiled object $name"
    }
    $existing = Join-Path $root 'initialized.exe'
    $before = [IO.File]::ReadAllBytes($existing)
    Run $linker ('"' + $existing + '" value "' + $root + '/initialized-0.c1o" "' + $root + '/initialized-1.c1o"') 1
    $after = [IO.File]::ReadAllBytes($existing)
    if ($before.Length -ne $after.Length) {
        throw 'Invalid entry truncated destination'
    }
    for ($i = 0; $i -lt $before.Length; $i++) {
        if ($before[$i] -ne $after[$i]) {
            throw 'Invalid entry changed destination'
        }
    }
    'PASS data entry rejected with initializer startup present'
    $source = Join-Path $root 'syntax-rejected.c'
    $object = Join-Path $root 'syntax-rejected.c1o'
    [IO.File]::WriteAllText($source, 'int main(void){return ; invalid syntax }')
    $env:C1_OUTPUT = 'object'
    $rejected = $false
    try {
        Compile $tool $source $object
    }
    catch {
        $rejected = $true
    }
    if (-not $rejected -or (Get-Item -LiteralPath $object).Length -ne 0) {
        throw 'Invalid syntax produced output'
    }
    'PASS object syntax rejected without output'
}
finally {
    $env:C1_OUTPUT = $old
}
