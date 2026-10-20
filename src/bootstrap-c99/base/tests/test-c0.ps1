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
    if ($Exe -ne $Compiler -and $script:TestEnvironment) {
        foreach ($name in $script:TestEnvironment.Keys) {
            $info.EnvironmentVariables[$name] = $script:TestEnvironment[$name]
        }
    }
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
        $executionLimit = if ($Exe -eq $Compiler -or [IO.Path]::GetExtension($InputFile) -eq '.c' -or [IO.Path]::GetFileName($Exe) -eq 'tmpnam-full-promised-limit.exe') {
            120000
        }
        else {
            10000
        }
        if (-not $p.WaitForExit($executionLimit)) {
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
        if ($CompilerKind -eq 'C1' -and !$CapacityBridgeOnly -and [IO.File]::ReadAllText($e) -notmatch '^C1 error: ') {
            throw "$Name missing C1 diagnostic" 
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

Check 'constant' 'int main(void){return 42;}' 42
Check 'precedence' 'int main(void){return 2+3*4-8/2;}' 10
Check 'parentheses' 'int main(void){return (2+3)*(9-4);}' 25
Check 'left-associative' 'int main(void){return 20-5-3;}' 12
Check 'division' 'int main(void){return -23/5;}' -4
Check 'remainder' 'int main(void){return -23%5;}' -3
Check 'literals' 'int main(void){return 0x2A+010;}' 50
Check 'characters' "int main(void){return 'A'+'\n'+'\t'+'\0';}" 84
Check 'bitwise' 'int main(void){return ((0x55 & 0x0f) | 0x20) ^ 3;}' 38
Check 'shift' 'int main(void){return (3<<4)+(-8>>2);}' 46
Check 'unary' 'int main(void){return !0 + !3 + (~0 == -1) + +7;}' 9
Check 'comparisons' 'int main(void){return (2<3)+(3>2)+(2<=2)+(3>=3)+(2==2)+(2!=3);}' 6
Check 'signed-comparisons' 'int main(void){return (-1<1)+(-1<=1)+(1>-1)+(1>=-1);}' 4
Check 'locals' 'int main(void){int a=3,b=4; a=a+b; return a*b;}' 28
Check 'assignment-chain' 'int main(void){int a,b; a=b=7; return a+b;}' 14
Check 'short-and' 'int main(void){int a=0; 0 && (a=9); return a;}'
Check 'short-or' 'int main(void){int a=0; 4 || (a=9); return a;}'
Check 'logical-normalize' 'int main(void){return (7&&9)+(0||4)+(0&&8)+(0||0);}' 2
Check 'logical-side-effect' 'int main(void){int a=0; 1 && (a=3); 0 || (a=a+4); return a;}' 7
Check 'if-else' 'int main(void){int a=2; if(a>3) a=11; else a=22; return a;}' 22
Check 'dangling-else' 'int main(void){int a=0;if(1)if(0)a=1;else a=2;return a;}' 2
Check 'while' 'int main(void){int i=0,s=0;while(i<10){s=s+i;i=i+1;}return s;}' 45
Check 'while-control' 'int main(void){int i=0,s=0;while(1){i=i+1;if(i==3)continue;if(i==8)break;s=s+i;}return s;}' 25
Check 'for' 'int main(void){int i,s=0;for(i=0;i<10;i=i+1)s=s+i;return s;}' 45
Check 'for-control' 'int main(void){int i,s=0;for(i=0;i<10;i=i+1){if(i==2)continue;if(i==7)break;s=s+i;}return s;}' 19
Check 'for-empty' 'int main(void){int i=0;for(;;){i=i+1;if(i==7)break;}return i;}' 7
Check 'nested-loops' 'int main(void){int i,j,s=0;for(i=0;i<3;i=i+1){j=0;while(j<4){j=j+1;if(j==2)continue;s=s+1;}}return s;}' 9
Check 'four-arguments' 'int f(int a,int b,int c,int d){return a+10*b+100*c+1000*d;}int main(void){return f(1,2,3,4);}' 4321
Check 'nested-calls' 'int f(int a,int b){return a*10+b;}int main(void){return 3+f(f(1,2),f(3,4));}' 157
Check 'forward-call' 'int main(void){return f(21);}int f(int n){return n*2;}' 42
Check 'prototype' 'int f(int n);int main(void){return f(17);}int f(int n){return n+1;}' 18
Check 'recursion' 'int fib(int n){if(n<2)return n;return fib(n-1)+fib(n-2);}int main(void){return fib(10);}' 55
Check 'mutual-recursion' 'int odd(int n);int even(int n){if(n==0)return 1;return odd(n-1);}int odd(int n){if(n==0)return 0;return even(n-1);}int main(void){return even(10)+odd(9);}' 2
Check 'empty-return-default' 'int main(void){;}'
Check 'comments' "/* start */ int main(void){ // comment`nreturn 42; /* tail */ }" 42
Check 'smoke' (Get-Content (Join-Path $PSScriptRoot 'c0-smoke.c') -Raw) 120
Check 'global-zero' 'int g;int main(void){return g;}'
Check 'global-update' 'int g=0;int add(int n){g=g+n;return g;}int main(void){add(3);return add(4);}' 7
Check 'global-array' 'int a[10];int main(void){int i,s=0;for(i=0;i<10;i=i+1)a[i]=i*i;for(i=0;i<10;i=i+1)s=s+a[i];return s;}' 285
Check 'array-chain' 'int a[4],b[4];int main(void){a[1]=b[2]=17;return a[1]+b[2];}' 34
Check 'array-index-expression' 'int a[10];int main(void){a[2+3]=41;return a[10/2]+1;}' 42
Check 'array-call-rhs' 'int a[4];int f(int n){return n*7;}int main(void){a[2]=f(6);return a[2];}' 42
Check 'large-bss' 'int a[262144];int main(void){a[262143]=37;return a[0]+a[262143];}' 37
Check 'global-after-function' 'int f(void){return 1;}int a[2];int main(void){a[0]=f();return a[0];}' 1
Check 'getchar-eof' 'int main(void){return __c0_getchar();}' -1
Check 'putchar' 'int main(void){return __c0_putchar(65);}' 65 $false '' 'A'
Check 'getchar' 'int main(void){return __c0_getchar();}' 90 $false 'Z' ''
Check 'echo' 'int main(void){int c;while((c=__c0_getchar())!=-1)__c0_putchar(c);return 0;}' 0 $false "Hello, bootstrap!`n" "Hello, bootstrap!`n"
Check 'reverse' (Get-Content (Join-Path $PSScriptRoot 'c0-reverse.c') -Raw) 0 $false 'abcdef' 'fedcba'

Check 'no-main' 'int f(void){return 1;}' 0 $true
Check 'main-parameters' 'int main(int x){return x;}' 0 $true
Check 'undeclared-local' 'int main(void){return missing;}' 0 $true
Check 'undefined-function' 'int main(void){return missing(1);}' 0 $true
Check 'duplicate-local' 'int main(void){int x,x;return 0;}' 0 $true
Check 'duplicate-function' 'int main(void){return 0;}int main(void){return 1;}' 0 $true
Check 'arity-call' 'int f(int a){return a;}int main(void){return f(1,2);}' 0 $true
Check 'arity-definition' 'int main(void){return f(1);}int f(int a,int b){return a+b;}' 0 $true
Check 'five-parameters' 'int f(int a,int b,int c,int d,int e){return a;}int main(void){return 0;}' 0 ($CompilerKind -eq 'C0')
Check 'trailing-argument' 'int f(int a){return a;}int main(void){return f(1,);}' 0 $true
Check 'trailing-parameter' 'int main(int a,){return 0;}' 0 $true
Check 'break-outside' 'int main(void){break;return 0;}' 0 $true
Check 'continue-outside' 'int main(void){continue;return 0;}' 0 $true
Check 'nested-declaration' 'int main(void){{int a;}return 0;}' 0 ($CompilerKind -eq 'C0')
Check 'invalid-lvalue' 'int main(void){1=3;return 0;}' 0 $true
Check 'missing-semicolon' 'int main(void){return 42}' 0 $true
Check 'unterminated-comment' 'int main(void){/*' 0 $true
Check 'unterminated-block' 'int main(void){return 0;' 0 $true
Check 'invalid-octal' 'int main(void){return 08;}' 0 $true
Check 'empty-hex' 'int main(void){return 0x;}' 0 $true
Check 'integer-range-overflow' 'int main(void){return 18446744073709551616;}' 0 $true
Check 'pointer-declaration' 'int main(void){int *p;return 0;}' 0 ($CompilerKind -eq 'C0')
Check 'long-name' ('int main(void){int ' + ('a' * 64) + ';return 0;}') 0 $true
Check 'deep-parentheses' ('int main(void){return ' + ('(' * 150) + '1' + (')' * 150) + ';}') 0 $true
Check 'deep-unary' ('int main(void){return ' + ('!' * 150) + '1;}') 0 $true
Check 'deep-statements' ('int main(void){' + ('if(1)' * 150) + 'return 0;}') 0 $true
if ($CompilerKind -eq 'C0') {
    Check 'nonzero-global-initializer' 'int g=1;int main(void){return g;}' 0 $true 
}
else {
    Check 'nonzero-global-initializer' 'int g=1;int main(void){return g;}' 1 
}
Check 'zero-array' 'int a[0];int main(void){return 0;}' 0 $true
Check 'array-value' 'int a[2];int main(void){return a;}' 0 $true
Check 'global-duplicate' 'int g;int g;int main(void){return 0;}' 0 ($CompilerKind -eq 'C0')
Check 'global-function-collision' 'int g;int g(void){return 1;}int main(void){return 0;}' 0 $true
Check 'builtin-redefinition' 'int __c0_getchar(void){return 0;}int main(void){return 0;}' 0 $true
Check 'builtin-arity' 'int main(void){return __c0_putchar();}' 0 $true
Check 'reserved-c89-keyword' 'int main(void){int switch;return 0;}' 0 $true
Check 'reserved-c99-keyword' 'int main(void){int restrict;return 0;}' 0 $true
if ($CompilerKind -eq 'C1') {
    function PpCheck([string]$Name, [string[]]$Lines, [int]$Result = 0, [bool]$Reject = $false) {
        Check $Name ([string]::Join([string][char]10, $Lines)) $Result $Reject
    }
    function PpString([string]$Name, [string[]]$Lines, [string]$Expression, [string]$Expected) {
        $program = 'int main(void){char *p=' + $Expression + ';while(*p){__c0_putchar(*p);p++;}return 0;}'
        Check $Name ([string]::Join([string][char]10, ($Lines + @($program)))) 0 $false '' $Expected
    }
    PpString 'pp-string-basic' @('#define S(x) #x') 'S(abc)' 'abc'
    PpString 'pp-string-no-prescan' @('#define A 7', '#define S(x) #x') 'S(A)' 'A'
    PpString 'pp-string-two-level' @('#define A 7', '#define S(x) #x', '#define X(x) S(x)') 'X(A)' '7'
    PpString 'pp-string-empty' @('#define S(x) #x') 'S()' ''
    PpString 'pp-string-space-collapse' @('#define S(x) #x') 'S(  a   +  b  )' 'a + b'
    PpString 'pp-string-no-invented-space' @('#define S(x) #x') 'S(a+b)' 'a+b'
    PpString 'pp-string-comment' @('#define S(x) #x') 'S(a/**/b)' 'a b'
    PpString 'pp-string-newline' @('#define S(x) #x') ('S(a' + [char]10 + 'b)') 'a b'
    PpString 'pp-string-literal-quotes' @('#define S(x) #x') 'S("a")' '"a"'
    PpString 'pp-string-literal-escape' @('#define S(x) #x') ('S("a' + [char]92 + 'n")') ('"a' + [char]92 + 'n"')
    PpString 'pp-string-character-escape' @('#define S(x) #x') ('S(''' + [char]92 + 'n'')') ('''' + [char]92 + 'n''')
    PpString 'pp-string-call-spelling' @('#define S(x) #x') 'S(S(abc))' 'S(abc)'
    PpString 'pp-string-unused-invalid-macro' @('#define BAD(x) x', '#define S(x) #x') 'S(BAD(1,2))' 'BAD(1,2)'
    PpString 'pp-string-raw-and-expanded' @('#define A 7', '#define F(x) #x "=" S(x)', '#define S(x) #x') 'F(A)' 'A=7'
    PpString 'pp-string-expanded-and-raw' @('#define A 7', '#define F(x) S(x) "=" #x', '#define S(x) #x') 'F(A)' '7=A'
    PpString 'pp-string-generated-space' @('#define PLUS +', '#define S(x) #x', '#define X(x) S(x)') 'X(a PLUS b)' 'a + b'
    PpString 'pp-string-generated-adjacency' @('#define PLUS +', '#define S(x) #x', '#define X(x) S(x)') 'X(a+PLUS)' 'a++'
    PpString 'pp-string-empty-macro-space' @('#define EMPTY', '#define S(x) #x', '#define X(x) S(x)') 'X(a EMPTY+b)' 'a +b'
    PpString 'pp-string-digraph' @('%:define S(x) %:x') 'S(abc)' 'abc'
    PpCheck 'pp-paste-name' @('#define CAT(a,b) a##b', 'int xy=7;int main(void){return CAT(x,y);}') 7
    PpCheck 'pp-paste-number' @('#define CAT(a,b) a##b', 'int main(void){return CAT(12,34);}') 1234
    PpCheck 'pp-paste-hex' @('#define CAT(a,b) a##b', 'int main(void){return CAT(0x,2a);}') 42
    PpCheck 'pp-paste-suffix' @('#define CAT(a,b) a##b', 'int main(void){return sizeof CAT(1,ULL);}') 8
    PpCheck 'pp-paste-operator' @('#define CAT(a,b) a##b', 'int main(void){return 1 CAT(<,=) 2;}') 1
    PpCheck 'pp-paste-no-prescan' @('#define A x', '#define AB 7', '#define CAT(a,b) a##b', 'int main(void){return CAT(A,B);}') 7
    PpCheck 'pp-paste-two-level' @('#define A x', '#define CAT(a,b) a##b', '#define X(a,b) CAT(a,b)', 'int xy=7;int main(void){return X(A,y);}') 7
    PpCheck 'pp-paste-rescan-function' @('#define CAT(a,b) a##b', '#define FN(x) ((x)+1)', 'int main(void){return CAT(F,N)(7);}') 8
    PpCheck 'pp-paste-empty-left' @('#define CAT(a,b) a##b', 'int main(void){return CAT(,7);}') 7
    PpCheck 'pp-paste-empty-right' @('#define CAT(a,b) a##b', 'int main(void){return CAT(7,);}') 7
    PpCheck 'pp-paste-empty-both' @('#define CAT(a,b) a##b', 'int main(void){return 7 CAT(,);}') 7
    PpCheck 'pp-paste-empty-middle' @('#define CAT(a,b,c) a##b##c', 'int xy=7;int main(void){return CAT(x,,y);}') 7
    PpCheck 'pp-paste-empty-chain' @('#define CAT(a,b,c) a##b##c', 'int main(void){return 7 CAT(,,);}') 7
    PpCheck 'pp-paste-multiple-token-arguments' @('#define CAT(a,b) a##b', 'int xy=7;int main(void){return CAT(1+x,y+2);}') 10
    PpCheck 'pp-paste-digraph' @('%:define CAT(a,b) a %:%: b', 'int xy=7;int main(void){return CAT(x,y);}') 7
    PpString 'pp-paste-generated-hash-token' @('#define HASH # ## #', '#define S(x) #x', '#define X(x) S(x)') 'X(HASH)' '##'
    PpString 'pp-paste-wide-token-spelling' @('#define CAT(a,b) a##b', '#define S(x) #x', '#define X(x) S(x)') 'X(CAT(L,"a"))' 'L"a"'
    PpString 'pp-paste-pp-number-spelling' @('#define CAT(a,b) a##b', '#define S(x) #x', '#define X(x) S(x)') 'X(CAT(1e,+2))' '1e+2'
    PpString 'pp-string-result-stringized' @('#define S(x) #x', '#define X(x) S(x)') 'X(S(abc))' '"abc"'
    PpCheck 'pp-paste-condition' @('#define CAT(a,b) a##b', '#if CAT(1,2)==12', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-reject-hash-nonparameter' @('#define F(x) #y', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-hash-at-end' @('#define F(x) x #', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-paste-at-start' @('#define F(x) ## x', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-paste-at-end' @('#define F(x) x ##', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-object-paste-end' @('#define F x ##', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-paste-invalid-operator' @('#define CAT(a,b) a##b', 'int main(void){return CAT(+,*);} ') 0 $true
    PpCheck 'pp-reject-paste-comment' @('#define CAT(a,b) a##b', 'int main(void){return CAT(/,*);} ') 0 $true
    PpCheck 'pp-reject-paste-two-strings' @('#define CAT(a,b) a##b', 'int main(void){return CAT("a","b")[0];}') 0 $true
    PpCheck 'pp-reject-stringize-arity' @('#define BAD(x) x', '#define F(x) #x BAD(x)', 'int main(void){return F(1,2);}') 0 $true
    PpCheck 'pp-string-image-baseline' @('int main(void){return "abc"[0];}') 97
    PpCheck 'pp-string-image-cleared' @('#define S(x) #x', 'int main(void){return S(abc)[0];}') 97
    if ((Get-FileHash (Join-Path $ScratchDirectory 'pp-string-image-baseline.exe')).Hash -ne (Get-FileHash (Join-Path $ScratchDirectory 'pp-string-image-cleared.exe')).Hash) {
        throw 'Generated token storage leaked into PE bytes'
    }

    PpCheck 'pp-object' @('#define X 7', 'int main(void){return X;}') 7
    PpCheck 'pp-object-rescan' @('#define X Y+2', '#define Y 5', 'int main(void){return X;}') 7
    PpCheck 'pp-object-late-definition' @('#define X Y', '#define Y 9', 'int main(void){return X;}') 9
    PpCheck 'pp-object-repeated' @('#define X 3', 'int main(void){return X+X+X;}') 9
    PpCheck 'pp-object-keyword' @('#define int long', 'signed main(void){int x;return sizeof x;}') 4
    PpCheck 'pp-object-empty' @('#define EMPTY', 'int main(EMPTY void){return 3 EMPTY;}') 3
    PpCheck 'pp-object-parentheses' @('#define X (3+4)', 'int main(void){return X*2;}') 14
    PpCheck 'pp-object-string' @('#define X "hello"', 'int main(void){return X[1];}') 101
    PpCheck 'pp-no-expand-in-literals' @('#define X 3', 'int main(void){return "X"[0]+''X'';}') 176
    PpCheck 'pp-object-boundary' @('#define PLUS +', 'int main(void){return 3 PLUS + 2;}') 5
    PpCheck 'pp-object-self' @('#define X X', 'int X=7;int main(void){return X+X;}') 14
    PpCheck 'pp-object-mutual' @('#define X Y', '#define Y X', 'int X=7;int main(void){return X;}') 7
    PpCheck 'pp-object-undef' @('#define X 7', 'int f(void){return X;}', '#undef X', 'int X=3;int main(void){return f()+X;}') 10
    PpCheck 'pp-object-redefine-after-undef' @('#define X 2', '#undef X', '#define X 9', 'int main(void){return X;}') 9
    PpCheck 'pp-object-identical-redefine' @('#define X 1 + 2', '#define X    1 /* space */ + 2', 'int main(void){return X;}') 3
    PpCheck 'pp-object-leading-trailing-space' @('#define X 1+2', '#define X    1+2   ', 'int main(void){return X;}') 3
    PpCheck 'pp-object-comment-in-define' @('#define X /* a', 'b */ 7', 'int main(void){return X;}') 7
    PpCheck 'pp-object-splice' @(('#define X 3+' + [char]92), '4', 'int main(void){return X;}') 7
    PpCheck 'pp-directive-splice' @(('#defi' + [char]92), 'ne X 9', 'int main(void){return X;}') 9
    PpCheck 'pp-directive-digraph' @('%:define X 6', 'int main(void){return X;}') 6
    PpCheck 'pp-directive-trigraph' @('??=define X 5', 'int main(void){return X;}') 5
    PpCheck 'pp-directive-leading-comment' @(' /* comment */ #define X 8', 'int main(void){return X;}') 8
    PpCheck 'pp-null-pragma' @('#', '#pragma unknown ignored', '#undef UNDEFINED', 'int main(void){return 7;}') 7
    PpCheck 'pp-ifdef' @('#define X', '#ifdef X', 'int main(void){return 7;}', '#else', 'bad tokens', '#endif') 7
    PpCheck 'pp-ifndef' @('#ifndef X', 'int main(void){return 9;}', '#else', 'bad tokens', '#endif') 9
    PpCheck 'pp-if-arithmetic' @('#define X 3', '#if X*4+2==14', 'int main(void){return 7;}', '#else', '#error wrong', '#endif') 7
    PpCheck 'pp-if-defined' @('#define X', '#if defined(X) && defined X && !defined(Y)', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-undefined-keyword' @('#if missing || sizeof || int', '#error wrong', '#else', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-elif' @('#if 0', '#error wrong', '#elif 0', '#error wrong', '#elif 2', 'int main(void){return 7;}', '#else', '#error wrong', '#endif') 7
    PpCheck 'pp-if-skipped-elif' @('#if 1', 'int main(void){return 7;}', '#elif 1/0', '#error wrong', '#endif') 7
    PpCheck 'pp-if-nested-inactive' @('#if 0', '#if 1/0', '#error wrong', '#else', 'invalid @ tokens', '#endif', '#endif', 'int main(void){return 7;}') 7
    PpCheck 'pp-if-ignored-definitions' @('#if 0', '#define X 7', '#unknown ignored', '#endif', '#ifdef X', '#error wrong', '#endif', 'int main(void){return 3;}') 3
    PpCheck 'pp-if-definition-in-branch' @('#if 1', '#define X 9', '#endif', 'int main(void){return X;}') 9
    PpCheck 'pp-if-short-circuit' @('#if (0 && 1/0) || (1 || (1<<100))', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-conditional' @('#if (0 ? 1/0 : 7)==7', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-intmax-decimal' @('#if 2147483648 > 0', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-intmax-hex' @('#if 0xffffffff > -1', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-uintmax' @('#if 18446744073709551615U > 9223372036854775807LL', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-comparison-width' @('#if ~(0==0) == -2', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-if-character' @('#if ''A''==65', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-unused-pp-number' @('#define UNUSED 0x1e+2', 'int main(void){return 7;}') 7
    PpCheck 'pp-reject-malformed-pp-number' @('int main(void){return 0x1e+2;}') 0 $true
    PpCheck 'pp-reject-redefine' @('#define X 1', '#define X 2', 'int main(void){return X;}') 0 $true
    PpCheck 'pp-reject-redefine-whitespace' @('#define X 1+2', '#define X 1 +2', 'int main(void){return X;}') 0 $true
    PpCheck 'pp-reject-define-name' @('#define 2 3', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-undef-extra' @('#undef X Y', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-active-error' @('#error intentional', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-active-include' @('#include "missing.h"', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-function-basic' @('#define F(x) (x)', 'int main(void){return F(3);}') 3
    PpCheck 'pp-function-multiple' @('#define ADD(x,y) ((x)+(y))', 'int main(void){return ADD(3,4);}') 7
    PpCheck 'pp-function-zero' @('#define VALUE() 7', 'int main(void){return VALUE();}') 7
    PpCheck 'pp-function-empty-argument' @('#define F(x) (3 x)', 'int main(void){return F();}') 3
    PpCheck 'pp-function-empty-two' @('#define F(x,y) (3 x y)', 'int main(void){return F(,);} ') 3
    PpCheck 'pp-function-empty-result' @('#define F(x)', 'int main(void){return 3 F(7);}') 3
    PpCheck 'pp-function-argument-prescan' @('#define X 3', '#define F(a) ((a)+(a))', 'int main(void){return F(X+1);}') 8
    PpCheck 'pp-function-nested-self' @('#define F(x) ((x)+(x))', 'int main(void){return F(F(3));}') 12
    PpCheck 'pp-function-nested-other' @('#define F(x) ((x)+1)', '#define G(x) F(x)', 'int main(void){return G(F(3));}') 5
    PpCheck 'pp-function-argument-parentheses' @('#define F(x) (x)', 'int main(void){return F((2,7));}') 7
    PpCheck 'pp-function-unused-unexpanded' @('#define BAD(x) x', '#define IGNORE(x) 7', 'int main(void){return IGNORE(BAD(1,2));}') 7
    PpCheck 'pp-function-unused-pp-number' @('#define IGNORE(x) 7', 'int main(void){return IGNORE(0x1e+2);}') 7
    PpCheck 'pp-function-parameter-literal' @('#define F(x) ("x"[0]+''x'')', 'int main(void){return F(7);}') 240
    PpCheck 'pp-function-multiline-call' @('#define F(x,y) ((x)+(y))', 'int main(void){return F', '(', '3,', '4', ');}') 7
    PpCheck 'pp-function-comment-call' @('#define F(x) (x)', 'int main(void){return F /* gap */ (7);}') 7
    PpCheck 'pp-function-not-called' @('#define F(x) ((x)+1)', 'int F=7;int main(void){return F;}') 7
    PpCheck 'pp-function-object-boundary' @('#define NAME F', '#define F(x) ((x)+1)', 'int main(void){return NAME(7);}') 8
    PpCheck 'pp-function-returned-name' @('#define F() G', '#define G(x) ((x)+1)', 'int main(void){return F()(7);}') 8
    PpCheck 'pp-function-parameter-call-boundary' @('#define CALL(x) x(7)', '#define F(x) ((x)+1)', 'int main(void){return CALL(F);}') 8
    PpCheck 'pp-function-argument-name-boundary' @('#define ID(x) x', '#define F(x) ((x)+1)', 'int main(void){return ID(F)(7);}') 8
    PpCheck 'pp-function-prescan-hidden-name' @('#define F(x) x', '#define G F', 'int (F)(int x){return x+10;}int main(void){return F(G)(2);}') 12
    PpCheck 'pp-function-closing-hide-intersection' @('#define F(x) x', '#define G(x) F(x)', 'int (F)(int x){return x+10;}int main(void){return F(G)(2);}') 2
    PpCheck 'pp-function-opening-boundary' @('#define OPEN F(', '#define F(x) ((x)+1)', 'int main(void){return OPEN 7);}') 8
    PpCheck 'pp-function-empty-inner' @('#define NOTHING()', '#define F(x) (7 x)', 'int main(void){return F(NOTHING());}') 7
    PpCheck 'pp-function-self-suppression' @('#define F(x) F(x+1)', 'int (F)(int x){return x;}int main(void){return F(7);}') 8
    PpCheck 'pp-function-mutual-suppression' @('#define F(x) G(x+1)', '#define G(x) F(x+1)', 'int (F)(int x){return x;}int main(void){return F(7);}') 9
    PpCheck 'pp-function-hidden-after-argument' @('#define F(x) (x+1)', '#define PASS(x) x', 'int F=7;int main(void){return PASS(F)(2);}') 3
    PpCheck 'pp-function-comma-expansion' @('#define PAIR 2,7', '#define F(x) (x)', 'int main(void){return F(PAIR);}') 7
    PpCheck 'pp-function-identical-definition' @('#define F(x,y) ((x)+(y))', '#define F( x , y ) ((x)+(y))', 'int main(void){return F(3,4);}') 7
    PpCheck 'pp-function-undef-redefine' @('#define F(x) x', '#undef F', '#define F 7', 'int main(void){return F;}') 7
    PpCheck 'pp-function-keyword-parameter' @('#define F(int) ((int)+1)', 'int main(void){return F(7);}') 8
    PpCheck 'pp-function-conditional' @('#define F(x) ((x)+1)', '#if F(2)==3', 'int main(void){return 7;}', '#else', '#error wrong', '#endif') 7
    PpCheck 'pp-function-conditional-defined' @('#define F(x) (x)', '#if F(defined(F))', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-function-conditional-name' @('#define F(x) (x)', '#if F', '#error wrong', '#else', 'int main(void){return 7;}', '#endif') 7
    PpCheck 'pp-function-repeat-stream' @('#define F(x) ((x)+1)', 'int main(void){return F(1)+F(2)+F(3);}') 9
    PpCheck 'pp-function-reject-too-few' @('#define F(x,y) x', 'int main(void){return F(1);}') 0 $true
    PpCheck 'pp-function-reject-too-many' @('#define F(x) x', 'int main(void){return F(1,2);}') 0 $true
    PpCheck 'pp-function-reject-zero-argument-token' @('#define EMPTY', '#define F() 7', 'int main(void){return F(EMPTY);}') 0 $true
    PpCheck 'pp-function-reject-duplicate-parameter' @('#define F(x,x) x', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-function-reject-trailing-comma' @('#define F(x,) x', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-function-reject-unclosed-call' @('#define F(x) x', 'int main(void){return F(1;}') 0 $true
    PpCheck 'pp-function-reject-object-redefinition' @('#define F(x) x', '#define F (x)', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-function-reject-parameter-rename' @('#define F(x) x', '#define F(y) y', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-function-stringize' @('#define F(x) #x', 'int main(void){return F(abc)[0];}') 97
    PpCheck 'pp-function-variadic' @('#define F(...) 7', 'int main(void){return F();}') 7
    $macroNames = (0..126 | ForEach-Object { 'p' + $_ }) -join ','
    $macroValues = (0..126 | ForEach-Object { [string]$_ }) -join ','
    PpCheck 'pp-function-127-parameters' @(('#define F(' + $macroNames + ') (p0+p63+p126)'), ('int main(void){return F(' + $macroValues + ');}')) 189
    PpCheck 'pp-function-reject-128-parameters' @(('#define F(' + $macroNames + ',extra) 7'), 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-object-paste' @('#define X a##b', 'int main(void){int ab=7;return X;}') 7
    PpCheck 'pp-reject-else-without-if' @('#else', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-duplicate-else' @('#if 1', '#else', '#else', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-elif-after-else' @('#if 1', '#else', '#elif 0', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-unclosed-if' @('#if 1', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-endif-extra' @('#if 1', '#endif X', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-ifdef-extra' @('#ifdef X Y', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-defined-operand' @('#if defined(1)', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-if-empty' @('#if', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-if-division' @('#if 1/0', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-if-sizeof' @('#if sizeof(int)', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-if-cast' @('#if (int)1', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-if-comma' @('#if 1,2', '#endif', 'int main(void){return 0;}') 0 $true
    PpCheck 'pp-reject-generated-directive' @('#define HASH #', 'HASH define X 1', 'int main(void){return 0;}') 0 $true
    . (Join-Path $PSScriptRoot 'test-includes.ps1')
    . (Join-Path $PSScriptRoot 'test-variadic-macros.ps1')
    . (Join-Path $PSScriptRoot 'test-locations.ps1')
    . (Join-Path $PSScriptRoot 'test-time-macros.ps1')
    . (Join-Path $PSScriptRoot 'test-pragma.ps1')
    . (Join-Path $PSScriptRoot 'test-diagnostics.ps1')
    . (Join-Path $PSScriptRoot 'test-diagnostic-reasons.ps1')
    . (Join-Path $PSScriptRoot 'test-flexible-arrays.ps1')
    . (Join-Path $PSScriptRoot 'test-integer-modes.ps1')
    . (Join-Path $PSScriptRoot 'test-core-headers.ps1')
    . (Join-Path $PSScriptRoot 'test-ctype.ps1')
    . (Join-Path $PSScriptRoot 'test-stdint.ps1')
    . (Join-Path $PSScriptRoot 'test-integer-library.ps1')
    . (Join-Path $PSScriptRoot 'test-memory.ps1')
    . (Join-Path $PSScriptRoot 'test-const-types.ps1')
    . (Join-Path $PSScriptRoot 'test-string-library.ps1')
    . (Join-Path $PSScriptRoot 'test-integer-conversion.ps1')
    . (Join-Path $PSScriptRoot 'test-search-library.ps1')
    . (Join-Path $PSScriptRoot 'test-locale-library.ps1')
    . (Join-Path $PSScriptRoot 'test-variadic-functions.ps1')
    . (Join-Path $PSScriptRoot 'test-volatile-restrict.ps1')
    . (Join-Path $PSScriptRoot 'test-bitfields.ps1')
    . (Join-Path $PSScriptRoot 'test-null-constants.ps1')
    . (Join-Path $PSScriptRoot 'test-floating.ps1')
    . (Join-Path $PSScriptRoot 'test-math.ps1')
    . (Join-Path $PSScriptRoot 'test-runtime-extensions.ps1')
    . (Join-Path $PSScriptRoot 'test-stdio.ps1')
    $capacityMain = 'int main(void){return 7;}'
    Check 'capacity-source-256k' ((' ' * 262144) + $capacityMain) 7
    Check 'capacity-source-limit' ($capacityMain + (' ' * (1048576 - $capacityMain.Length))) 7
    Check 'capacity-source-over-limit' ($capacityMain + (' ' * (1048577 - $capacityMain.Length))) 0 $true
    Check 'capacity-string-256k' ('int main(void){char *p="' + ('a' * 262144) + '";return p[262143]+p[262144];}') 97
    Check 'capacity-code-over-256k' ('int main(void){' + ('1;' * 120000) + 'return 9;}') 9
    if ((Get-Item -LiteralPath (Join-Path $ScratchDirectory 'capacity-code-over-256k.exe')).Length -le 262656) {
        throw 'Large code test did not cross the old output limit'
    }
    Check 'capacity-code-over-limit' ('int main(void){' + ('1;' * 240000) + 'return 9;}') 0 $true
    Check 'capacity-bss-limit-io' 'int a[6815744];int main(void){a[6815743]=37;__c0_putchar(65);return a[0]+a[6815743];}' 37 $false '' 'A'
    Check 'capacity-static-large' 'int main(void){static int a[4194304];a[4194303]=19;return a[0]+a[4194303];}' 19
    Check 'capacity-bss-over-limit' 'int a[6815745];int main(void){return 0;}' 0 $true
    Check 'block-function-later-file-typedef' 'int g(void){int f(int);return sizeof f(2);}typedef long long f;int main(void){return g()+sizeof(f);}' 12
    Check 'block-function-later-file-enum' 'int g(void){int f(int);return sizeof f(2);}enum E{f=7};int main(void){return g()+f;}' 11
    Check 'block-function-later-typedef-reshadow' 'int g(void){int f(int);return sizeof f(2);}typedef long long f;int main(void){int r;{int f(int);r=sizeof f(3);}return g()+r+sizeof(f);}' 16
    Check 'block-function-later-enum-reshadow' 'int g(void){int f(int);return sizeof f(2);}enum E{f=7};int main(void){int r;{int f(int);r=sizeof f(3);}return g()+r+f;}' 15
    Check 'block-function-forward' 'int main(void){int f(int);return f(7);}int f(int n){return n+2;}' 9
    Check 'block-function-extern' 'int main(void){extern int f(int);return f(7);}int f(int n){return n+2;}' 9
    Check 'block-function-typedef' 'typedef int F(int);int main(void){F f;return f(8);}int f(int n){return n;}' 8
    Check 'block-function-pointer-return' 'int a=9;int main(void){int *f(void);return *f();}int *f(void){return &a;}' 9
    Check 'block-function-wide-return' 'int main(void){long long f(void);return f()==4294967297LL;}long long f(void){return 4294967297LL;}' 1
    Check 'block-function-void' 'int a;int main(void){void f(int);f(8);return a;}void f(int n){a=n;}' 8
    Check 'block-function-aggregate' 'struct S{int a,b,c;};int main(void){struct S f(struct S);struct S s={2,3,4};s=f(s);return s.a+s.b+s.c;}struct S f(struct S s){s.b+=7;return s;}' 16
    Check 'block-function-mixed-local' 'int main(void){int a=3,f(int),b=5;return f(a)+b;}int f(int n){return n*2;}' 11
    Check 'block-function-mixed-extern' 'int main(void){extern int a,f(int),b;return f(a)+b;}int a=3,b=5;int f(int n){return n*2;}' 11
    Check 'block-function-repeat' 'int main(void){int f(int);extern int f(int);int f();return f(8);}int f(int n){return n;}' 8
    Check 'block-function-shadow-local' 'int main(void){int f=3,r;{int f(int);r=f(8);}return r+f;}int f(int n){return n;}' 11
    Check 'block-function-shadow-typedef' 'typedef int f;int main(void){int r;{int f(int);r=sizeof f(8);}return r+sizeof(f);}' 8
    Check 'block-function-shadow-enum' 'enum E{f=3};int main(void){int r;{int f(int);r=sizeof f(8);}return r+f;}' 7
    Check 'block-function-static-linkage' 'static int f(int);int main(void){extern int f(int);return f(8);}static int f(int n){return n;}' 8
    Check 'block-function-address' 'int main(void){int f(int);int (*p)(int)=&f;return p(8);}int f(int n){return n;}' 8
    Check 'block-function-static-address' 'int main(void){int f(int);static int (*p)(int)=f;return p(8);}int f(int n){return n;}' 8
    Check 'block-function-no-prototype-leak' 'int f();int main(void){{int f(int);if(sizeof f(1)!=4)return 1;}return sizeof f(1,2);}int f(int n){return n;}' 4
    Check 'block-function-no-cross-body-leak' 'int g(void){int f(int);return sizeof f(1);}int main(void){return g()+sizeof f(1,2);}int f(int n){return n;}' 8
    Check 'block-function-compatible-bodies' 'int g(void){int f(int);return f(2);}int main(void){int f(int);return g()+f(3);}int f(int n){return n;}' 5
    Check 'block-function-incomplete-return' 'typedef int A[];A *f(void);int main(void){{int (*f(void))[3];return sizeof *f();}}' 12
    Check 'block-function-reject-address-leak' 'int main(void){{int f(int);}return &f!=0;}int f(int n){return n;}' 0 $true
    Check 'block-function-reject-cross-body-address' 'int g(void){int f(int);return 0;}int main(void){return &f!=0;}int f(int n){return n;}' 0 $true
    Check 'block-function-reject-inherited-arity' 'int f(int);int main(void){int f();return sizeof f(1,2);}' 0 $true
    Check 'block-function-reject-completion-leak' 'typedef int A[];A *f(void);int main(void){{int (*f(void))[3];}return sizeof *f();}' 0 $true
    Check 'block-function-reject-auto' 'int main(void){auto int f(int);return 0;}' 0 $true
    Check 'block-function-reject-register' 'int main(void){register int f(int);return 0;}' 0 $true
    Check 'block-function-reject-static' 'int main(void){static int f(int);return 0;}' 0 $true
    Check 'block-function-reject-for' 'int main(void){for(int f(int);0;){}return 0;}' 0 $true
    Check 'block-function-reject-init' 'int main(void){int f(int)=0;return 0;}' 0 $true
    Check 'block-function-reject-body' 'int main(void){int f(int n){return n;}return 0;}' 0 $true
    Check 'block-function-reject-local-conflict' 'int main(void){int f;int f(int);return 0;}' 0 $true
    Check 'block-function-reject-typedef-conflict' 'int main(void){typedef int f;int f(int);return 0;}' 0 $true
    Check 'block-function-reject-global-object' 'int f;int main(void){int f(int);return 0;}' 0 $true
    Check 'block-function-reject-hidden-static' 'static int f(int);int main(void){int f;{extern int f(int);}return 0;}' 0 $true
    Check 'block-function-reject-entity-conflict' 'int g(void){int f(int);return 0;}int main(void){int f(long);return 0;}' 0 $true
    Check 'block-function-reject-parameter-count' 'int main(void){int f(int);return f();}int f(int n){return n;}' 0 $true
    Check 'block-function-inferred-probe' 'struct S{int n;};int f(int n){return n;}int main(void){struct S a[]={{f(2)},{f(3)}};return a[0].n+a[1].n;}' 5
    Check 'function-null-local' 'int main(void){int (*p)(int)=(void *)0;return p==0;}' 1
    Check 'function-null-global' 'int (*p)(int)=(void *)0;int main(void){return p==0;}' 1
    Check 'function-null-static' 'int main(void){static int (*p)(int)=(void *)0;return p==0;}' 1
    Check 'function-null-static-ice' 'int (*p)(int)=(void *)(3-3);int main(void){return p==0;}' 1
    Check 'function-null-integer-cast' 'int main(void){int (*p)(int)=(void *)(unsigned long long)0;return p==0;}' 1
    Check 'function-null-assignment' 'int f(int n){return n;}int main(void){int (*p)(int)=f;p=(void *)0;return p==0;}' 1
    Check 'function-null-argument' 'int f(int (*p)(int)){return p==0;}int main(void){return f((void *)0);}' 1
    Check 'function-null-return' 'typedef int F(int);F *f(void){return (void *)0;}int main(void){return f()==0;}' 1
    Check 'function-null-compare-right' 'int f(int n){return n;}int main(void){return f!=(void *)0;}' 1
    Check 'function-null-compare-left' 'int f(int n){return n;}int main(void){return (void *)0!=f;}' 1
    Check 'function-null-equal' 'int main(void){int (*p)(int)=0;return p==(void *)0;}' 1
    Check 'function-null-conditional-right' 'int f(int n){return n;}int main(void){return (1?f:(void *)0)(7);}' 7
    Check 'function-null-conditional-left' 'int f(int n){return n;}int main(void){return (0?(void *)0:f)(7);}' 7
    Check 'function-null-conditional-zero' 'int f(int n){return n;}int main(void){int (*p)(int)=0?f:(void *)0;return p==0;}' 1
    Check 'function-null-static-conditional-right' 'int f(int n){return n;}int (*p)(int)=1?f:(void *)0;int main(void){return p(7);}' 7
    Check 'function-null-static-conditional-left' 'int f(int n){return n;}int (*p)(int)=0?(void *)0:f;int main(void){return p(7);}' 7
    Check 'function-null-static-conditional-zero' 'int f(int n){return n;}int (*p)(int)=0?f:(void *)0;int main(void){return p==0;}' 1
    Check 'function-null-array' 'int f(int n){return n;}int (*a[])(int)={(void *)0,f};int main(void){return (a[0]==0)+a[1](6);}' 7
    Check 'function-null-reject-variable' 'int main(void){void *q=0;int (*p)(int)=q;return 0;}' 0 $true
    Check 'function-null-reject-variable-compare' 'int f(int n){return n;}int main(void){void *q=0;return f==q;}' 0 $true
    Check 'function-null-reject-recast' 'int main(void){int (*p)(int)=(void *)(int *)0;return 0;}' 0 $true
    Check 'function-null-reject-static-recast' 'int (*p)(int)=(void *)(int *)0;int main(void){return 0;}' 0 $true
    Check 'function-null-reject-reverse-conversion' 'int main(void){void *p=(int (*)(int))0;return 0;}' 0 $true
    Check 'function-null-reject-conditional-variable' 'int f(int n){return n;}int main(void){void *q=0;int (*p)(int)=1?f:q;return 0;}' 0 $true
    Check 'function-null-reject-order' 'int f(int n){return n;}int main(void){return f<(void *)0;}' 0 $true
    Check 'complex-old-function-pointer' 'int f(int n){return n;}int (*choose(n))(int) int n;{return f;}int main(void){return choose(0)(7);}' 7
    Check 'complex-old-default-int' 'int f(int n){return n;}int (*choose(n))(int){if(n==3)return f;return 0;}int main(void){return choose(3)(7);}' 7
    Check 'complex-old-array-pointer' 'int a[3]={1,2,7};int (*get(n))[3] int n;{return &a;}int main(void){return (*get(0))[2];}' 7
    Check 'complex-old-narrow' 'int f(int n){return n;}int (*choose(n))(int) char n;{if(n==-1)return f;return 0;}int main(void){return choose(255)(7);}' 7
    Check 'complex-old-bool' 'int f(int n){return n;}int (*choose(n))(int) _Bool n;{if(n==1)return f;return 0;}int main(void){return choose(256)(7);}' 7
    Check 'complex-old-prototype' 'int (*choose(int))(int);int f(int n){return n;}int (*choose(n))(int) char n;{return f;}int main(void){return choose(3)(7);}' 7
    Check 'complex-old-prototype-after' 'int f(int n){return n;}int (*choose(n))(int) char n;{return f;}int (*choose(int))(int);int main(void){return choose(3)(7);}' 7
    Check 'complex-old-reordered' 'int f(int n){return n;}int (*choose(a,b))(int) int b,a;{if(a*10+b==34)return f;return 0;}int main(void){return choose(3,4)(7);}' 7
    Check 'complex-old-register' 'int f(int n){return n;}int (*choose(n))(int) register n;{if(n==3)return f;return 0;}int main(void){return choose(3)(7);}' 7
    Check 'complex-old-stack' 'int f(int n){return n;}int (*choose(a,b,c,d,e))(int) int a,b,c,d,e;{if(a+b+c+d+e==15)return f;return 0;}int main(void){return choose(1,2,3,4,5)(7);}' 7
    Check 'complex-old-callback' 'int f(int n){return n;}int (*choose(p))(int) int p(int);{return p;}int main(void){return choose(f)(7);}' 7
    Check 'complex-old-aggregate' 'struct S{int a[3];};int f(int n){return n;}int (*choose(s))(int) struct S s;{if(s.a[0]==3)return f;return 0;}int main(void){struct S s={{3}};return choose(s)(7);}' 7
    Check 'complex-old-recursion' 'int f(int n){return n;}int (*choose(n))(int) int n;{if(n)return choose(n-1);return f;}int main(void){return choose(3)(7);}' 7
    Check 'complex-old-return-prototype-scope' 'int f(int n){return n;}int (*choose(n))(int n) int n;{if(n==3)return f;return 0;}int main(void){return choose(3)(7);}' 7
    Check 'complex-old-reject-identifier-prototype' 'int (*choose(n))(int);int main(void){return 0;}' 0 $true
    Check 'complex-old-reject-duplicate' 'int (*choose(n))(int) int n;char n;{return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-old-reject-extra' 'int (*choose(n))(int) int n,x;{return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-old-reject-storage' 'int (*choose(n))(int) static int n;{return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-old-reject-initializer' 'int (*choose(n))(int) int n=3;{return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-old-reject-prototype-type' 'int (*choose(char))(int);int (*choose(n))(int) char n;{return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-function-pointer' 'int f(int n){return n+1;}int (*choose(int n))(int){return f;}int main(void){return choose(0)(6);}' 7
    Check 'complex-def-array-pointer' 'int a[3]={1,2,7};int (*get(void))[3]{return &a;}int main(void){return (*get())[2];}' 7
    Check 'complex-def-parameter-value' 'int f(int n){return n+1;}int g(int n){return n+2;}int (*choose(int n))(int){return n?f:g;}int main(void){return choose(0)(5);}' 7
    Check 'complex-def-prototype-before' 'int (*choose(int))(int);int f(int n){return n;}int (*choose(int n))(int){return f;}int main(void){return choose(0)(7);}' 7
    Check 'complex-def-prototype-after' 'int f(int n){return n;}int (*choose(int n))(int){return f;}int (*choose(int))(int);int main(void){return choose(0)(7);}' 7
    Check 'complex-def-typedef-prototype' 'typedef int F(int);F *choose(int);int f(int n){return n;}int (*choose(int n))(int){return f;}int main(void){return choose(0)(7);}' 7
    Check 'complex-def-empty-list' 'int f(int n){return n;}int (*choose())(int){return f;}int main(void){return choose()(7);}' 7
    Check 'complex-def-void-prototype-empty-definition' 'int (*choose(void))(int);int f(int n){return n;}int (*choose())(int){return f;}int main(void){return choose()(7);}' 7
    Check 'complex-def-register' 'int f(int n){return n;}int (*choose(register int n))(int){if(n)return f;return 0;}int main(void){return choose(1)(7);}' 7
    Check 'complex-def-callback-parameter' 'int f(int n){return n;}int (*choose(int (*p)(int)))(int){return p;}int main(void){return choose(f)(7);}' 7
    Check 'complex-def-parameter-array' 'int a[3]={1,2,7};int (*get(int p[3]))[3]{return &a;}int main(void){return (*get(a))[2];}' 7
    Check 'complex-def-stack-parameters' 'int values[3]={1,2,7};int (*get(int a,int b,int c,int d,int e,int f))[3]{if(a+b+c+d+e+f==21)return &values;return 0;}int main(void){return (*get(1,2,3,4,5,6))[2];}' 7
    Check 'complex-def-aggregate-parameter' 'struct S{int a[3];};int values[3]={1,2,7};int (*get(struct S s))[3]{if(s.a[0]==7)return &values;return 0;}int main(void){struct S s={{7}};return (*get(s))[2];}' 7
    Check 'complex-def-enum-scope' 'int values[3]={1,2,7};int (*get(enum E{N=7} n))[3]{if(n==N)return &values;return 0;}int main(void){return (*get(7))[2];}' 7
    Check 'complex-def-return-prototype-name-scope' 'int f(int n){return n;}int (*choose(int n))(int n){if(n==3)return f;return 0;}int main(void){return choose(3)(7);}' 7
    Check 'complex-def-return-prototype-tag-scope' 'int f(int n){return n;}int (*choose(int n))(enum E{N=7}){return f;}enum E{N=3};int main(void){return choose(0)(7);}' 7
    Check 'complex-def-local-shadow' 'int f(int n){return n;}int (*choose(int n))(int){{int n=9;}if(n==3)return f;return 0;}int main(void){return choose(3)(7);}' 7
    Check 'complex-def-recursion' 'int f(int n){return n;}int (*choose(int n))(int){if(n)return choose(n-1);return f;}int main(void){return choose(3)(7);}' 7
    Check 'complex-def-pointer-to-pointer-return' 'int f(int n){return n;}int (*p)(int)=f;int (**get(void))(int){return &p;}int main(void){return (*get())(7);}' 7
    Check 'complex-def-static' 'int f(int n){return n;}static int (*choose(int n))(int){return f;}int main(void){return choose(0)(7);}' 7
    Check 'complex-def-reject-unnamed' 'int (*choose(int))(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-register-address' 'int (*choose(register int n))(int){int *p=&n;return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-return-scope-leak' 'int (*choose(void))(enum E{N=7}){int n=N;return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-parameter-scope-leak' 'int (*choose(enum E{N=7} n))(int){return 0;}int main(void){return N;}' 0 $true
    Check 'complex-def-reject-prototype-arity' 'int (*choose(int))(int);int (*choose())(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-type-conflict' 'int (*choose(long))(int);int (*choose(int n))(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-duplicate-definition' 'int (*choose(void))(int){return 0;}int (*choose(void))(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-typedef-name-conflict' 'typedef int choose;int (*choose(int choose))(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'complex-def-reject-incomplete-param' 'struct S;int (*choose(struct S s))(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'typed-decl-typedef-function' 'typedef int F(int);F f;int main(void){return f(7);}int f(int n){return n;}' 7
    Check 'typed-decl-repeat' 'typedef int F(int);F f;F f;int f(int n){return n;}int main(void){return f(7);}' 7
    Check 'typed-decl-after-prototype' 'typedef int F(int);int f(int);F f;int f(int n){return n;}int main(void){return f(7);}' 7
    Check 'typed-decl-after-definition' 'typedef int F(int);int f(int n){return n;}F f;int main(void){return f(7);}' 7
    Check 'typed-decl-extern' 'typedef int F(int);extern F f;int f(int n){return n;}int main(void){return f(7);}' 7
    Check 'typed-decl-static' 'typedef int F(int);static F f;int f(int n){return n;}int main(void){return f(7);}' 7
    Check 'typed-decl-static-inherit' 'typedef int F(int);static int f(int);F f;int f(int n){return n;}int main(void){return f(7);}' 7
    Check 'typed-decl-forward-address' 'typedef int F(int);F f;F *p=f;int main(void){return p(7);}int f(int n){return n;}' 7
    Check 'typed-decl-complex-return-function-pointer' 'int (*choose(int))(int);int f(int n){return n+1;}typedef int F(int);F *choose(int n){return f;}int main(void){return choose(0)(6);}' 7
    Check 'typed-decl-complex-return-array-pointer' 'int (*get(void))[3];int a[3]={1,2,7};typedef int A[3];A *get(void){return &a;}int main(void){return (*get())[2];}' 7
    Check 'typed-decl-function-pointer-return-typedef' 'typedef int F(int);typedef F *G(void);G choose;int f(int n){return n;}F *choose(void){return f;}int main(void){return choose()(7);}' 7
    Check 'typed-decl-multiple-functions' 'int f(int),g(int);int f(int n){return n+1;}int g(int n){return n+2;}int main(void){return f(2)+g(2);}' 7
    Check 'typed-decl-multiple-typedef-functions' 'typedef int F(int);F f,g;int f(int n){return n+1;}int g(int n){return n+2;}int main(void){return f(2)+g(2);}' 7
    Check 'typed-decl-function-then-object' 'int f(int),x=3;int f(int n){return n+1;}int main(void){return f(3)+x;}' 7
    Check 'typed-decl-object-then-function' 'int x=3,f(int);int f(int n){return n+1;}int main(void){return f(3)+x;}' 7
    Check 'typed-decl-object-function-pointer-mix' 'int x=3,f(int),(*p)(int)=f;int f(int n){return n+1;}int main(void){return p(3)+x;}' 7
    Check 'typed-decl-pointer-return-base' 'int a=7;int *f(void),g(int);int *f(void){return &a;}int g(int n){return n;}int main(void){return g(*f());}' 7
    Check 'typed-decl-parameter-name-scope' 'typedef int T;int f(int T),g(T);T x=7;int f(int n){return n;}int g(int n){return n;}int main(void){return g(x);}' 7
    Check 'typed-decl-unspecified' 'typedef int F();F f;int main(void){return f(7);}int f(int n){return n;}' 7
    Check 'typed-decl-void' 'typedef int F(void);F f;int f(void){return 7;}int main(void){return f();}' 7
    Check 'typed-decl-oldstyle-compatible' 'int f(n) char n;{return n;}typedef int F(int);F f;int main(void){return f(7);}' 7
    Check 'typed-decl-reject-oldstyle-mismatch' 'int f(n) long n;{return n;}typedef int F(int);F f;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-return-conflict' 'typedef long F(int);int f(int);F f;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-parameter-conflict' 'typedef int F(long);int f(int);F f;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-object-conflict' 'typedef int F(int);int f;F f;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-initializer' 'typedef int F(int);F f=0;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-linkage-conflict' 'typedef int F(int);int f(int);static F f;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-unspecified-char' 'typedef int F();int f(char);F f;int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-mixed-incompatible' 'int f(int),f(long);int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-function-return-function' 'typedef int F(int);F f(void);int main(void){return 0;}' 0 $true
    Check 'typed-decl-reject-trailing-comma' 'int f(int),;int main(void){return 0;}' 0 $true
    Check 'indirect-local' 'int f(int n){return n+1;}int main(void){int (*p)(int)=f;return p(6);}' 7
    Check 'indirect-address' 'int f(int n){return n+1;}int main(void){int (*p)(int)=&f;return p(6);}' 7
    Check 'indirect-global' 'int f(int n){return n+1;}int (*p)(int)=f;int main(void){return p(6);}' 7
    Check 'indirect-forward-address' 'int f(int);int (*p)(int)=&f;int main(void){return p(6);}int f(int n){return n+1;}' 7
    Check 'indirect-dereference' 'int f(int n){return n+1;}int main(void){int (*p)(int)=f;return (*p)(6);}' 7
    Check 'indirect-repeat-dereference' 'int f(int n){return n+1;}int main(void){return (***f)(6);}' 7
    Check 'indirect-address-dereference' 'int f(int n){return n+1;}int main(void){return (&*f)(6);}' 7
    Check 'indirect-parenthesized-function' 'int f(int n){return n+1;}int main(void){return (f)(6);}' 7
    Check 'indirect-callback' 'int f(int n){return n+1;}int apply(int p(int),int n){return p(n);}int main(void){return apply(f,6);}' 7
    Check 'indirect-typedef' 'typedef int F(int);int f(int n){return n+1;}int apply(F *p,int n){return p(n);}int main(void){return apply(f,6);}' 7
    Check 'indirect-array' 'int f(int n){return n+1;}int g(int n){return n+2;}int (*a[2])(int)={f,g};int main(void){return a[1](5);}' 7
    Check 'indirect-member' 'int f(int n){return n+1;}struct S{int (*p)(int);};struct S s={f};int main(void){return s.p(6);}' 7
    Check 'indirect-arrow-member' 'int f(int n){return n+1;}struct S{int (*p)(int);};struct S s={f};int main(void){struct S *p=&s;return p->p(6);}' 7
    Check 'indirect-return-pointer' 'typedef int F(int);int f(int n){return n+1;}F *choose(void){return f;}int main(void){return choose()(6);}' 7
    Check 'indirect-conditional' 'int f(int n){return n+1;}int g(int n){return n+2;}int main(void){return (0?f:g)(5);}' 7
    Check 'indirect-comma' 'int f(int n){return n+1;}int main(void){int n=0;return (n++,f)(6)+n;}' 8
    Check 'indirect-target-once' 'int f(int n){return n+1;}int main(void){int (*a[2])(int)={f,f};int i=0;int n=a[i++](6);return n+i;}' 8
    Check 'indirect-target-before-arguments' 'typedef int F(int);int f(int n){return n+1;}int g(int n){return n+2;}F *p=f;int change(void){p=g;return 6;}int main(void){return p(change());}' 7
    Check 'indirect-nested' 'int f(int n){return n+1;}int main(void){int (*p)(int)=f;return p(p(p(4)));}' 7
    Check 'indirect-five-args' 'int f(int a,int b,int c,int d,int e){return a+b+c+d+e;}int main(void){int (*p)(int,int,int,int,int)=f;return p(1,2,3,4,5);}' 15
    Check 'indirect-wide-return' 'long long f(long long n){return n<<40;}int main(void){long long (*p)(long long)=f;return p(7)>>40;}' 7
    Check 'indirect-void-return' 'int n;void f(int x){n=x;}int main(void){void (*p)(int)=f;p(7);return n;}' 7
    Check 'indirect-narrow-conversion' 'int f(char n){return n;}int main(void){int (*p)(char)=f;return p(255);}' -1
    Check 'indirect-aggregate-parameter' 'struct S{int a[3];};int f(struct S s){s.a[0]++;return s.a[0];}int main(void){struct S s={{3}};int (*p)(struct S)=f;return p(s)+s.a[0];}' 7
    Check 'indirect-aggregate-return' 'struct S{int a[3];};struct S f(int n){struct S s={{n}};return s;}int main(void){struct S (*p)(int)=f;return p(7).a[0];}' 7
    Check 'indirect-small-aggregate-return' 'struct S{int n;};struct S f(int n){struct S s={n};return s;}int main(void){struct S (*p)(int)=f;return p(7).n;}' 7
    Check 'indirect-aggregate-stack-mix' 'struct S{int a[3];};struct S f(int a,int b,int c,int d,int e){struct S s={{a+b+c+d+e}};return s;}int main(void){struct S (*p)(int,int,int,int,int)=f;return p(1,2,3,4,5).a[0];}' 15
    Check 'indirect-static-local' 'int f(int n){return n+1;}int main(void){static int (*p)(int)=f;return p(6);}' 7
    Check 'indirect-oldstyle' 'int f(n) char n;{return n;}int main(void){int (*p)()=f;return p(7);}' 7
    Check 'indirect-oldstyle-prototype' 'int f(n) char n;{return n;}int main(void){int (*p)(int)=f;return p(255);}' -1
    Check 'indirect-function-equality' 'int f(int n){return n;}int main(void){return f==&f;}' 1
    Check 'indirect-function-truth' 'int f(int n){return n;}int main(void){return !!f;}' 1
    Check 'indirect-sizeof-pointer' 'int f(int);int main(void){return sizeof &f;}' 8
    Check 'indirect-sizeof-call' 'int f(int);int main(void){int (*p)(int)=0;return sizeof p(7);}' 4
    Check 'indirect-sizeof-no-target-call' 'typedef int F(int);int n;int f(int x){return x;}F *choose(void){n++;return f;}int main(void){return sizeof choose()(7)+n;}' 4
    Check 'indirect-probe-address' 'int f(int n){return n;}struct S{int (*p)(int);};int main(void){struct S a[]={f,f};return a[1].p(7);}' 7
    Check 'indirect-reject-function-sizeof' 'int f(int n){return n;}int main(void){return sizeof f;}' 0 $true
    Check 'indirect-reject-deref-sizeof' 'int main(void){int (*p)(int)=0;return sizeof *p;}' 0 $true
    Check 'indirect-reject-arity' 'int f(int n){return n;}int main(void){int (*p)(int)=f;return p();}' 0 $true
    Check 'indirect-reject-type' 'int f(int *p){return *p;}int main(void){int (*p)(int *)=f;return p(3);}' 0 $true
    Check 'indirect-reject-function-assignment' 'int f(int n){return n;}int g(int n){return n;}int main(void){f=g;return 0;}' 0 $true
    Check 'indirect-reject-undefined-address' 'int f(int);int (*p)(int)=f;int main(void){return 0;}' 0 $true
    Check 'indirect-reject-wrong-signature' 'long f(int n){return n;}int (*p)(int)=f;int main(void){return 0;}' 0 $true
    Check 'indirect-reject-static-call' 'int f(void){return 7;}int n=(*f)();int main(void){return n;}' 0 $true
    Check 'indirect-builtin' 'int main(void){int (*p)(int)=__c0_putchar;return p(65);}' 65 $false '' 'A'
    Check 'function-type-local-pointer' 'int main(void){int (*p)(int)=0;return sizeof p+(p==0);}' 9
    Check 'function-type-global-pointer' 'int (*p)(int)=0;int main(void){return p==0;}' 1
    Check 'function-type-typedef' 'typedef int F(int);int main(void){F *p=0;return sizeof p;}' 8
    Check 'function-type-pointer-typedef' 'typedef int (*P)(int);int main(void){P p=0;return p==0;}' 1
    Check 'function-type-array' 'int (*a[3])(int);int main(void){return sizeof a+(a[2]==0);}' 25
    Check 'function-type-member' 'struct S{int (*p)(int);int n;};struct S s={0,7};int main(void){return s.n+(s.p==0);}' 8
    Check 'function-type-union' 'union U{int (*p)(int);long long n;};union U u={0};int main(void){return u.p==0;}' 1
    Check 'function-type-copy' 'int main(void){int (*a)(int)=0;int (*b)(int)=a;return a==b;}' 1
    Check 'function-type-parameter' 'int f(int (*p)(int)){return p==0;}int main(void){return f(0);}' 1
    Check 'function-type-parameter-adjust' 'int f(int p(int)){return p==0;}int main(void){return f(0);}' 1
    Check 'function-type-typedef-parameter-adjust' 'typedef int F(int);int f(F p){return p==0;}int main(void){return f(0);}' 1
    Check 'function-type-return-pointer' 'typedef int F(int);F *f(void){return 0;}int main(void){return f()==0;}' 1
    Check 'function-type-abstract-sizeof' 'int main(void){return sizeof(int (*)(int));}' 8
    Check 'function-type-abstract-cast' 'int main(void){return (int (*)(int))0==0;}' 1
    Check 'function-type-void-prototype' 'int (*p)(void);int main(void){return sizeof p;}' 8
    Check 'function-type-unspecified' 'int (*p)();int (*p)(int);int main(void){return sizeof p;}' 8
    Check 'function-type-unspecified-reverse' 'int (*p)(int);int (*p)();int main(void){return sizeof p;}' 8
    Check 'function-type-nested-parameter' 'int (*p)(int (*)(int));int main(void){return sizeof p;}' 8
    Check 'function-type-nested-return' 'int (*(*p)(void))(int);int main(void){return sizeof p;}' 8
    Check 'function-type-param-array-compatible' 'int (*p)(int [3]);int (*p)(int *);int main(void){return sizeof p;}' 8
    Check 'function-type-parameter-names' 'int (*p)(int x,int y);int x=7;int main(void){return x;}' 7
    Check 'function-type-parameter-typedef-shadow' 'typedef int T;int (*p)(int T);T x=7;int main(void){return x;}' 7
    Check 'function-type-parameter-tag-scope' 'int (*p)(enum E{N=7});enum E{N=3};int main(void){return N;}' 3
    Check 'function-type-incomplete-parameter' 'struct S;int (*p)(struct S x);int main(void){return sizeof p;}' 8
    Check 'function-type-inferred-probe' 'struct S{int n;};int main(void){struct S a[]={sizeof(int (*)(int)),7};return sizeof a+a[0].n;}' 16
    Check 'function-type-reject-return-conflict' 'int (*p)(int);long (*p)(int);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-parameter-conflict' 'int (*p)(int);int (*p)(long);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-arity-conflict' 'int (*p)(int);int (*p)(int,int);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-unspecified-char' 'int (*p)();int (*p)(char);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-sizeof-function' 'typedef int F(int);int main(void){return sizeof(F);}' 0 $true
    Check 'function-type-reject-array-of-functions' 'typedef int F(int);F a[2];int main(void){return 0;}' 0 $true
    Check 'function-type-reject-function-return-function' 'typedef int F(int);typedef F G(int);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-function-return-array' 'typedef int A[3];typedef A F(int);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-increment' 'int (*p)(int);int main(void){p++;return 0;}' 0 $true
    Check 'function-type-reject-offset' 'int (*p)(int);int main(void){p=p+1;return 0;}' 0 $true
    Check 'function-type-reject-order' 'int (*p)(int);int (*q)(int);int main(void){return p<q;}' 0 $true
    Check 'function-type-reject-void-object-conversion' 'int main(void){void *p=0;int (*q)(int)=p;return 0;}' 0 $true
    Check 'function-type-reject-duplicate-param' 'int (*p)(int x,int x);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-param-storage' 'int (*p)(static int x);int main(void){return 0;}' 0 $true
    Check 'function-type-reject-identifier-list' 'int (*p)(x,y);int main(void){return 0;}' 0 $true
    Check 'oldstyle-explicit' 'int f(a,b) int a;int b;{return a+b;}int main(void){return f(3,4);}' 7
    Check 'oldstyle-default-parameters' 'int f(a,b){return a+b;}int main(void){return f(3,4);}' 7
    Check 'oldstyle-partial-default' 'int f(a,b) char b;{return a+b;}int main(void){return f(3,4);}' 7
    Check 'oldstyle-default-return' 'f(a) int a;{return a;}main(){return f(7);}' 7
    Check 'oldstyle-storage-default-return' 'static f(a) int a;{return a;}int main(void){return f(7);}' 7
    Check 'oldstyle-register-default' 'int f(a) register a;{return a;}int main(void){return f(7);}' 7
    Check 'oldstyle-reordered' 'int f(a,b) int b,a;{return a*10+b;}int main(void){return f(3,4);}' 34
    Check 'oldstyle-char-conversion' 'int f(a) signed char a;{return a;}int main(void){return f(255);}' -1
    Check 'oldstyle-ushort-conversion' 'int f(a) unsigned short a;{return a;}int main(void){return f(65537);}' 1
    Check 'oldstyle-bool-conversion' 'int f(a) _Bool a;{return a;}int main(void){return f(256);}' 1
    Check 'oldstyle-pointer' 'int f(a) int *a;{return *a;}int main(void){int a=7;return f(&a);}' 7
    Check 'oldstyle-array-adjustment' 'int f(a) int a[3];{return a[2];}int main(void){int a[3]={1,2,7};return f(a);}' 7
    Check 'oldstyle-array-pointer' 'int f(a) int (*a)[3];{return (*a)[2];}int main(void){int a[3]={1,2,7};return f(&a);}' 7
    Check 'oldstyle-prototype-before' 'int f(int,int);int f(a,b) char a;short b;{return a+b;}int main(void){return f(3,4);}' 7
    Check 'oldstyle-prototype-after' 'int f(a) char a;{return a;}int f(int);int main(void){return f(7);}' 7
    Check 'oldstyle-unspecified-before' 'int f();int f(a) char a;{return a;}int main(void){return f(7);}' 7
    Check 'oldstyle-unspecified-after' 'int f(a) char a;{return a;}int f();int main(void){return f(7);}' 7
    Check 'oldstyle-forward-call' 'int main(void){return f(7);}int f(a) char a;{return a;}' 7
    Check 'oldstyle-five-parameters' 'int f(a,b,c,d,e) int a,b,c,d,e;{return a+b+c+d+e;}int main(void){return f(1,2,3,4,5);}' 15
    Check 'oldstyle-recursion' 'int f(a) int a;{if(a)return f(a-1)+1;return 0;}int main(void){return f(7);}' 7
    Check 'oldstyle-aggregate' 'struct S{int a[3];};int f(s,n) struct S s;int n;{return s.a[0]+n;}int main(void){struct S s={{3}};return f(s,4);}' 7
    Check 'oldstyle-aggregate-bool' 'struct S{int a[3];};int f(s,n) struct S s;_Bool n;{return s.a[0]+n;}int main(void){struct S s={{3}};return f(s,256);}' 4
    Check 'oldstyle-aggregate-return' 'struct S{int a[3];};struct S f(n) int n;{struct S s={{n}};return s;}int main(void){return f(7).a[0];}' 7
    Check 'oldstyle-small-aggregate' 'struct S{int x;};int f(s) struct S s;{return s.x;}int main(void){struct S s={7};return f(s);}' 7
    Check 'oldstyle-enum' 'enum E{N=7};int f(n) enum E n;{return n;}int main(void){return f(N);}' 7
    Check 'oldstyle-local-shadow' 'int f(n) int n;{{int n=2;}return n;}int main(void){return f(7);}' 7
    Check 'oldstyle-reject-identifier-prototype' 'int f(a,b);int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-duplicate-name' 'int f(a,a) int a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-duplicate-declaration' 'int f(a) int a;char a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-extra-declaration' 'int f(a) int a,b;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-initializer' 'int f(a) int a=3;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-static' 'int f(a) static int a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-extern' 'int f(a) extern int a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-auto' 'int f(a) auto int a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-typedef' 'int f(a) typedef int a;{return 0;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-void' 'int f(a) void a;{return 0;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-prototype-char' 'int f(char);int f(a) char a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-later-prototype-type' 'int f(a) long long a;{return a;}int f(int);int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-prototype-count' 'int f(int,int);int f(a) int a;{return a;}int main(void){return 0;}' 0 $true
    Check 'oldstyle-reject-register-address' 'int f(a) register int a;{return *&a;}int main(void){return 0;}' 0 $true
    Check 'unspecified-before-int-prototype' 'int f();int f(int x){return x;}int main(void){return f(7);}' 7
    Check 'unspecified-call-before-definition' 'int f();int main(void){return f(7);}int f(int x){return x;}' 7
    Check 'unspecified-repeat' 'int f();int f();int main(void){return f(7);}int f(int x){return x;}' 7
    Check 'unspecified-after-prototype' 'int f(int);int f();int f(int x){return x;}int main(void){return f(7);}' 7
    Check 'unspecified-empty-definition' 'int f(){return 7;}int main(void){return f();}' 7
    Check 'unspecified-empty-then-void' 'int f(){return 7;}int f(void);int main(void){return f();}' 7
    Check 'unspecified-void-then-empty' 'int f(void);int f(){return 7;}int main(void){return f();}' 7
    Check 'unspecified-char-promotion' 'int f();int main(void){signed char x=-7;return f(x);}int f(int x){return x;}' -7
    Check 'unspecified-short-promotion' 'int f();int main(void){unsigned short x=65535;return f(x);}int f(int x){return x==65535;}' 1
    Check 'unspecified-bool-promotion' 'int f();int main(void){_Bool x=7;return f(x);}int f(int x){return x;}' 1
    Check 'unspecified-wide-argument' 'int f();int main(void){return f(1LL<<40);}int f(long long x){return x>>40;}' 1
    Check 'unspecified-wide-return' 'long long f();int main(void){return f(3)>>40;}long long f(int x){return (long long)x<<40;}' 3
    Check 'unspecified-pointer-return' 'int *f();int main(void){int n=7;return *f(&n);}int *f(int *p){return p;}' 7
    Check 'unspecified-void-return' 'int n;void f();int main(void){f(7);return n;}void f(int x){n=x;}' 7
    Check 'unspecified-aggregate-argument' 'struct S{int a[3];};int f();int main(void){struct S s={{7}};return f(s);}int f(struct S s){return s.a[0];}' 7
    Check 'unspecified-aggregate-return' 'struct S{int a[3];};struct S f();int main(void){return f(7).a[0];}struct S f(int n){struct S s={{n}};return s;}' 7
    Check 'unspecified-small-aggregate' 'struct S{int n;};struct S f();int main(void){return f(7).n;}struct S f(int n){struct S s={n};return s;}' 7
    Check 'unspecified-enum-compatible' 'enum E{N=7};int f();int main(void){return f(N);}int f(enum E x){return x;}' 7
    Check 'unspecified-five-arguments' 'int f();int main(void){return f(1,2,3,4,5);}int f(int a,int b,int c,int d,int e){return a+b+c+d+e;}' 15
    Check 'unspecified-unevaluated-different-counts' 'int f();int main(void){return sizeof f(1)+sizeof f(1,2);}int f(void){return 0;}' 8
    Check 'unspecified-unused-no-definition' 'long long f();int main(void){return sizeof f(1,2);}' 8
    Check 'unspecified-nested-implicit-calls' 'int main(void){return f(g(3));}int f(int x){return x+1;}int g(int x){return x+3;}' 7
    Check 'unspecified-reject-char-prototype' 'int f();int f(char x);int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-short-prototype' 'int f();int f(short x);int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-bool-prototype' 'int f();int f(_Bool x);int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-char-reverse' 'int f(char x);int f();int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-empty-definition-arity' 'int f(int);int f(){return 0;}int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-empty-definition-reverse' 'int f(){return 0;}int f(int);int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-return-conflict' 'long long f();int f(int x){return x;}int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-object-conflict' 'int f();int f;int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-typedef-conflict' 'int f();typedef int f;int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-enum-conflict' 'int f();enum E{f=1};int main(void){return 0;}' 0 $true
    Check 'unspecified-reject-prototype-retained' 'int f(int);int f();int main(void){return f();}int f(int x){return x;}' 0 $true
    Check 'unspecified-reject-runtime-count' 'int f();int main(void){return f(1,2);}int f(int x){return x;}' 0 $true
    Check 'unspecified-reject-implicit-char-definition' 'int main(void){return f(7);}int f(char x){return x;}' 0 $true
    Check 'param-incomplete-named-prototype' 'struct S;int f(struct S s);struct S{int n;};int f(struct S s){return s.n;}int main(void){struct S s={7};return f(s);}' 7
    Check 'param-incomplete-unused-prototype' 'struct S;int f(struct S s);int main(void){return 7;}' 7
    Check 'param-incomplete-union-prototype' 'union U;int f(union U u);union U{int n;};int f(union U u){return u.n;}int main(void){union U u={7};return f(u);}' 7
    Check 'param-large-named-prototype' 'struct S{char a[70000];};int f(struct S s);int main(void){return 7;}' 7
    Check 'param-large-register-prototype' 'struct S{char a[70000];};int f(register struct S s);int main(void){return 7;}' 7
    Check 'param-large-sizeof-prototype' 'struct S{char a[70000];};int f(struct S s,int a[sizeof s]);int main(void){return 7;}' 7
    Check 'param-incomplete-address-sizeof' 'struct S;int f(struct S s,int a[sizeof &s]);int main(void){return 7;}' 7
    Check 'param-completion-later-parameter' 'int f(struct S s,struct S{int a[3];} *p){return s.a[0]+p->a[0];}int main(void){return 7;}' 7
    Check 'param-layout-enum-binding' 'struct S{int a[3];};int f(enum E{N=3} n,struct S s,int x){int z=2;return n+s.a[0]+x+z;}int main(void){struct S s={{4}};return f(3,s,5);}' 14
    Check 'param-layout-block-restore' 'struct S{int a[3];};int f(struct S s,int x){int n=1;{int x=9;n+=x;}{int z=2;n+=z;}return n+s.a[0]+x;}int main(void){struct S s={{4}};return f(s,5);}' 21
    Check 'param-layout-address' 'struct S{int a[3];};int f(struct S s,int x){int *p=&x;return s.a[0]+*p;}int main(void){struct S s={{4}};return f(s,3);}' 7
    Check 'param-typedef-void-list' 'typedef void V;int f(V){return 7;}int main(void){return f();}' 7
    Check 'param-static-function-void-pointer' 'static int f(void *p){return *(int *)p;}int main(void){int n=7;return f(&n);}' 7
    Check 'param-register-unnamed-prototype' 'int f(register int);int f(int n){return n;}int main(void){return f(7);}' 7
    Check 'param-register-array' 'int f(register int a[3]){return a[2];}int main(void){int a[3]={1,2,7};return f(a);}' 7
    Check 'param-reject-incomplete-definition' 'struct S;int f(struct S s){return 0;}int main(void){return 0;}' 0 $true
    Check 'param-reject-large-definition' 'struct S{char a[70000];};int f(struct S s){return 0;}int main(void){return 0;}' 0 $true
    Check 'param-reject-incomplete-sizeof' 'struct S;int f(struct S s,int a[sizeof s]);int main(void){return 0;}' 0 $true
    Check 'param-reject-auto' 'int f(auto int n){return n;}int main(void){return 0;}' 0 $true
    Check 'param-reject-static' 'int f(static int n){return n;}int main(void){return 0;}' 0 $true
    Check 'param-reject-extern' 'int f(extern int n);int main(void){return 0;}' 0 $true
    Check 'param-reject-typedef' 'int f(typedef int n);int main(void){return 0;}' 0 $true
    Check 'param-reject-register-void' 'int f(register void);int main(void){return 0;}' 0 $true
    Check 'param-reject-named-void' 'int f(void x);int main(void){return 0;}' 0 $true
    Check 'param-reject-trailing-void' 'int f(int x,void);int main(void){return 0;}' 0 $true
    Check 'address-array-element' 'int a[4]={1,2,7,4};int *p=&a[2];int main(void){return *p;}' 7
    Check 'address-array-plus' 'int a[4]={1,2,7,4};int *p=a+2;int main(void){return *p;}' 7
    Check 'address-integer-plus' 'int a[4]={1,2,7,4};int *p=2+a;int main(void){return *p;}' 7
    Check 'address-integer-subscript' 'int a[4]={1,2,7,4};int *p=&2[a];int main(void){return *p;}' 7
    Check 'address-parenthesized' 'int a[4]={1,2,7,4};int *p=(&((a)[2]));int main(void){return *p;}' 7
    Check 'address-negative-offset' 'int a[4]={1,2,7,4};int *p=&a[3]+(-1);int main(void){return *p;}' 7
    Check 'address-subtract-offset' 'int a[4]={1,2,7,4};int *p=(a+4)-2;int main(void){return *p;}' 7
    Check 'address-array-pointer' 'int a[2][3]={{1,2,3},{4,5,7}};int (*p)[3]=&a[0]+1;int main(void){return (*p)[2];}' 7
    Check 'address-nested-subscript' 'int a[2][3]={{1,2,3},{4,5,7}};int *p=&a[1][2];int main(void){return *p;}' 7
    Check 'address-subarray-decay' 'int a[2][3]={{1,2,3},{4,5,7}};int *p=a[1]+2;int main(void){return *p;}' 7
    Check 'address-member-array' 'struct S{int a[3];int x;};struct S s={{1,2,7},4};int *p=s.a+2;int main(void){return *p;}' 7
    Check 'address-array-member' 'struct S{int x,y;};struct S a[2]={{1,2},{3,7}};int *p=&a[1].y;int main(void){return *p;}' 7
    Check 'address-nested-member' 'struct S{int x,y;};struct T{int z;struct S s;};struct T a={0,{3,7}};int *p=&a.s.y;int main(void){return *p;}' 7
    Check 'address-arrow' 'struct S{int x,y;};struct S s={3,7};int *p=&(&s)->y;int main(void){return *p;}' 7
    Check 'address-dereference' 'int a[3]={1,2,7};int *p=&*(a+2);int main(void){return *p;}' 7
    Check 'address-string-offset' 'char *p="abcdef"+3;int main(void){return *p;}' 100
    Check 'address-string-index' 'char *p=&"abcdef"[3];int main(void){return *p;}' 100
    Check 'address-string-concatenate' 'char *p=("abc" "def")+3;int main(void){return *p;}' 100
    Check 'address-cast-byte-offset' 'int a[3]={0,0,7};int *p=(int *)((char *)a+2*sizeof(int));int main(void){return *p;}' 7
    Check 'address-void-pointer' 'int a=7;void *p=(void *)&a;int main(void){return *(int *)p;}' 7
    Check 'address-null-cast' 'int *p=(void *)(3-3);int main(void){return p==0;}' 1
    Check 'address-fixed-integer-cast' 'int *p=(int *)0x123456789ULL;int main(void){return (unsigned long long)p==0x123456789ULL;}' 1
    Check 'address-conditional' 'int a=3,b=7;int *p=0?&a:&b;int main(void){return *p;}' 7
    Check 'address-conditional-null' 'int a=7;int *p=1?&a:0;int *q=0?0:&a;int main(void){return *p+*q;}' 14
    Check 'address-forward-array' 'extern int a[];int *p=a+2;int a[3]={1,2,7};int main(void){return *p;}' 7
    Check 'address-enum-offset' 'enum E{N=2};int a[3]={1,2,7};int *p=a+(N*2-2);int main(void){return *p;}' 7
    Check 'address-static-local' 'int main(void){static int a[3]={1,2,7};static int *p=&a[2];return *p;}' 7
    Check 'address-block-extern' 'int main(void){extern int a[3];static int *p=a+2;return *p;}int a[3]={1,2,7};' 7
    Check 'address-in-pointer-list' 'int a[3]={1,2,7};int *p[]={a+1,&a[2]};int main(void){return *p[0]+*p[1];}' 9
    Check 'address-in-record-list' 'int a[3]={1,2,7};struct S{int *p;int n;};struct S s={a+2,3};int main(void){return *s.p+s.n;}' 10
    Check 'address-offset-short-circuit' 'int a[3]={1,2,7};int *p=a+(1?2:1/0);int main(void){return *p;}' 7
    Check 'address-offset-shift' 'int a[3]={1,2,7};int *p=a+(1<<1);int main(void){return *p;}' 7
    Check 'address-reject-pointer-load' 'int a=7;int *p=&a;int *q=p;int main(void){return 0;}' 0 $true
    Check 'address-reject-index-load' 'int a[3];int n=2;int *p=a+n;int main(void){return 0;}' 0 $true
    Check 'address-reject-element-load' 'int a[3];int n=a[0];int main(void){return 0;}' 0 $true
    Check 'address-reject-member-load' 'struct S{int n;};struct S s={7};int n=s.n;int main(void){return 0;}' 0 $true
    Check 'address-reject-local-auto' 'int main(void){int a;static int *p=&a;return 0;}' 0 $true
    Check 'address-reject-call' 'int f(void){return 2;}int a[3];int *p=a+f();int main(void){return 0;}' 0 $true
    Check 'address-reject-multiply' 'int a[3];int *p=a*2;int main(void){return 0;}' 0 $true
    Check 'address-reject-void-arithmetic' 'int a;void *p=(void *)&a+1;int main(void){return 0;}' 0 $true
    Check 'address-reject-incompatible' 'int a[3];char *p=a+1;int main(void){return 0;}' 0 $true
    Check 'address-reject-offset-overflow' 'int a[3];int *p=a+(2147483647+1);int main(void){return 0;}' 0 $true
    Check 'init-record-array-fixed-copy' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[2]={s,s};return a[0].x+a[1].y;}' 7
    Check 'init-record-array-infer-copy' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[]={s,s};return sizeof a+a[1].y;}' 20
    Check 'init-record-array-infer-mixed' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[]={s,5,6,{7,8},s};return sizeof a+a[1].y+a[2].x;}' 45
    Check 'init-record-array-infer-once' 'struct S{int a[3];};int n;struct S make(void){struct S s={{++n,2,3}};return s;}int main(void){struct S a[]={make(),make()};return n*10+a[1].a[0];}' 22
    Check 'init-record-nested-copy' 'struct S{int x,y;};struct T{struct S s;int z;};int main(void){struct S s={3,4};struct T t={s,7};return t.s.y+t.z;}' 11
    Check 'init-record-deep-elision' 'struct S{int x,y;};struct T{struct S s;int z;};struct U{struct T t;int z;};int main(void){struct S s={3,4};struct U u={s,5,6};return u.t.s.y+u.t.z+u.z;}' 15
    Check 'init-record-infer-nested-array' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[][2]={s,s,s,s};return sizeof a+a[1][1].y;}' 36
    Check 'init-record-infer-nested-braces' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[][2]={{s,s},{s,s}};return sizeof a+a[1][1].y;}' 36
    Check 'init-record-union-first-copy' 'struct S{int x,y;};union U{struct S s;long long x;};int main(void){struct S s={3,4};union U a[]={s,s};return sizeof a+a[1].s.y;}' 20
    Check 'init-record-string-first' 'struct S{char a[4];int n;};int main(void){struct S a[]={"abc",3,"def",4};return sizeof a+a[1].n+a[0].a[1];}' 118
    Check 'init-record-string-pointer' 'struct S{char *p;int n;};int main(void){struct S a[]={"abc",3,"def",4};return a[1].p[0]+a[0].n;}' 103
    Check 'init-record-infer-conditional' 'struct S{int x,y;};int main(void){struct S s={3,4},t={5,6};struct S a[]={1?s:t,0?s:t};return sizeof a+a[1].y;}' 22
    Check 'init-record-infer-comma' 'struct S{int x,y;};int main(void){int n=0;struct S s={3,4};struct S a[]={(n++,s),(n++,s)};return sizeof a+n;}' 18
    Check 'init-record-infer-deref' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S *p=&s;struct S a[]={*p,*p};return sizeof a+a[0].x;}' 19
    Check 'init-record-infer-enum-checkpoint' 'struct S{int x,y;};int main(void){struct S a[]={sizeof(enum E{N=7}),N};enum E e=N;return sizeof a+a[0].x+a[0].y+e;}' 26
    Check 'init-record-infer-tag-checkpoint' 'struct S{int x,y;};int main(void){struct S a[]={sizeof(struct T{int x;}),7};struct T t={3};return sizeof a+a[0].x+t.x;}' 15
    Check 'init-record-infer-incomplete-checkpoint' 'struct S{int x,y;};int main(void){struct T;struct S a[]={sizeof(struct T{int x;}),7};struct T t={3};return sizeof a+a[0].x+t.x;}' 15
    Check 'init-record-infer-call-checkpoint' 'struct S{int x,y;};int main(void){struct S a[]={f(3),4};return sizeof a+a[0].x;}int f(int n){return n;}' 11
    Check 'init-record-infer-reference-checkpoint' 'struct S{int *p;int n;};extern int g;int main(void){struct S a[]={&g,3};return *a[0].p+a[0].n;}int g=4;' 7
    Check 'init-record-infer-multiple-declarators' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[]={s,s},b[]={s};return sizeof a+sizeof b;}' 24
    Check 'init-record-infer-self-sizeof-reject' 'struct S{int x,y;};int main(void){struct S a[]={sizeof a,1};return 0;}' 0 $true
    Check 'init-record-infer-wrong-copy-reject' 'struct S{int x,y;};struct T{int x,y;};int main(void){struct T t={3,4};struct S a[]={t};return 0;}' 0 $true
    Check 'init-record-scalar-top-reject' 'struct S{int x,y;};int main(void){struct S s=3;return 0;}' 0 $true
    Check 'init-record-fixed-excess-reject' 'struct S{int x,y;};int main(void){struct S s={3,4};struct S a[1]={s,s};return 0;}' 0 $true
    Check 'record-local-members' 'int main(void){struct S{int a;int b;} s;s.a=3;s.b=4;return s.a+s.b;}' 7
    Check 'record-global-members' 'struct S{int a;int b;} s;int main(void){s.a=7;return s.a+s.b;}' 7
    Check 'record-layout' 'struct S{char a;short b;char c;int d;};int main(void){struct S s;return sizeof s+(char*)&s.d-(char*)&s;}' 20
    Check 'record-wide-layout' 'struct S{char a;long long b;short c;};int main(void){struct S s;return sizeof s+(char*)&s.b-(char*)&s;}' 32
    Check 'record-pointer' 'struct S{int x;};int main(void){struct S s;struct S *p=&s;p->x=7;return s.x;}' 7
    Check 'record-pointer-function' 'struct S{int x;};int f(struct S *p){return p->x;}int main(void){struct S s={7};return f(&s);}' 7
    Check 'record-forward' 'struct S;struct S *p;struct S{int x;};struct S s={7};int main(void){p=&s;return p->x;}' 7
    Check 'record-tentative-incomplete' 'struct S s;struct S *p=&s;struct S{int x;};int main(void){p->x=7;return s.x;}' 7
    Check 'record-extern-later' 'struct S{int x;};extern struct S s;int main(void){return s.x;}struct S s={7};' 7
    Check 'record-self-pointer' 'struct N{int x;struct N *next;};struct N n={7,&n};int main(void){return n.next->next->x;}' 7
    Check 'record-nested' 'struct S{struct T{int x;} t;int y;};int main(void){struct S s={{3},4};return s.t.x+s.y;}' 7
    Check 'record-array-member' 'struct S{int a[3];};int main(void){struct S s={{1,2,7}};return s.a[2]+sizeof s.a;}' 19
    Check 'record-string-member' 'struct S{char s[4];int n;};struct S x={"ab",7};int main(void){return x.s[1]+x.n;}' 105
    Check 'record-array' 'struct S{int x;};int main(void){struct S a[2]={{3},{7}};return a[1].x+sizeof a;}' 15
    Check 'record-inferred-array' 'struct S{int x;int y;};struct S a[]={{1,2},{7}};int main(void){return sizeof a+a[1].x+a[1].y;}' 23
    Check 'record-flat-array' 'struct S{int x;int y;};int main(void){struct S a[]={1,2,7};return sizeof a+a[1].x+a[1].y;}' 23
    Check 'record-pointer-step' 'struct S{int x;};int main(void){struct S a[2]={{3},{7}};struct S *p=a;p++;return p->x;}' 7
    Check 'record-pointer-difference' 'struct S{char a;int b;};int main(void){struct S a[3];return &a[2]-&a[0];}' 2
    Check 'record-member-compound' 'struct S{int x;};int main(void){struct S s={2};s.x+=5;return s.x;}' 7
    Check 'record-member-postincrement' 'struct S{int x;};int main(void){struct S s={2};int n=s.x++;return n+s.x;}' 5
    Check 'record-arrow-evaluate-once' 'struct S{int x;};struct S a[2]={{2},{7}};int n;struct S *f(void){return &a[n++];}int main(void){f()->x+=3;return n+a[0].x;}' 6
    Check 'record-member-address' 'struct S{char a;int b;};int main(void){struct S s={1,7};int *p=&s.b;return *p;}' 7
    Check 'record-static-member-address' 'struct S{char a;int b;};struct S s={1,7};int *p=&s.b;int main(void){return *p;}' 7
    Check 'record-static-array-decay' 'struct S{int a[2];};struct S s={{3,7}};int *p=s.a;int main(void){return p[1];}' 7
    Check 'record-assignment' 'struct S{int a;int b;};int main(void){struct S a={3,4},b;b=a;return b.a+b.b;}' 7
    Check 'record-global-assignment' 'struct S{int a;int b;};struct S a={3,4},b;int main(void){b=a;return b.a+b.b;}' 7
    Check 'record-indirect-assignment' 'struct S{int a;int b;};int main(void){struct S a={3,4},b;struct S *p=&b;*p=a;return b.a+b.b;}' 7
    Check 'record-assignment-chain' 'struct S{int a;};int main(void){struct S a={7},b,c;c=b=a;return c.a;}' 7
    Check 'record-assignment-member-value' 'struct S{int a;};int main(void){struct S a={7},b;return (b=a).a;}' 7
    Check 'record-whole-initializer' 'struct S{int a;int b;};int main(void){struct S a={3,4};struct S b=a;return b.a+b.b;}' 7
    Check 'record-conditional' 'struct S{int x;};int main(void){struct S a={3},b={7},c;c=0?a:b;return c.x;}' 7
    Check 'record-sizeof-no-side-effects' 'struct S{int x;};int main(void){struct S a={3},b={7};int n=sizeof(a=b);return n+a.x;}' 7
    Check 'record-static-local' 'struct S{int x;};int f(void){static struct S s={2};return ++s.x;}int main(void){f();return f();}' 4
    Check 'record-shadow-tag' 'struct S{int x;};int main(void){int n;{struct S{char x;};n=sizeof(struct S);}return n+sizeof(struct S);}' 5
    Check 'record-member-name-space' 'typedef int x;struct S{x x;};int main(void){struct S s={7};return s.x+sizeof(x);}' 11
    Check 'record-enum-member' 'struct S{enum E{A=7} x;};int main(void){struct S s={A};return s.x;}' 7
    Check 'record-anonymous-typedef' 'typedef struct{int x;} S;int main(void){S s={7};return s.x;}' 7
    Check 'record-union-size' 'union U{char c;long long n;};int main(void){return sizeof(union U);}' 8
    Check 'record-union-members' 'union U{int n;char c[4];};int main(void){union U u={0x030201};return u.c[0]+u.c[1]+u.c[2];}' 6
    Check 'record-union-copy' 'union U{int n;char c[4];};int main(void){union U a={7},b;b=a;return b.n;}' 7
    Check 'record-union-nested' 'union U{struct S{int a;int b;} s;long long n;};union U u={{3,4}};int main(void){return u.s.a+u.s.b;}' 7
    Check 'record-register-read' 'struct S{int x;};int main(void){register struct S s={7};return s.x;}' 7
    Check 'record-register-array-sizeof' 'struct S{int a[3];};int main(void){register struct S s={{1,2,3}};return sizeof s.a;}' 12
    Check 'record-incomplete-address' 'struct S;extern struct S s;int main(void){return sizeof &s;}' 8
    Check 'record-reject-empty' 'struct S{};int main(void){return 0;}' 0 $true
    Check 'record-reject-self-member' 'struct S{struct S x;};int main(void){return 0;}' 0 $true
    Check 'record-reject-incomplete-sizeof' 'struct S;int main(void){return sizeof(struct S);}' 0 $true
    Check 'record-reject-duplicate-member' 'struct S{int x;char x;};int main(void){return 0;}' 0 $true
    Check 'record-reject-duplicate-definition' 'struct S{int x;};struct S{int y;};int main(void){return 0;}' 0 $true
    Check 'record-reject-recursive-definition' 'struct S{struct S{int x;} x;};int main(void){return 0;}' 0 $true
    Check 'record-reject-tag-kind' 'struct S{int x;};union S *p;int main(void){return 0;}' 0 $true
    Check 'record-reject-enum-tag-kind' 'struct S{int x;};enum S x;int main(void){return 0;}' 0 $true
    Check 'record-reject-member-missing' 'struct S{int x;};int main(void){struct S s;return s.y;}' 0 $true
    Check 'record-reject-arithmetic' 'struct S{int x;};int main(void){struct S s;return s+1;}' 0 $true
    Check 'record-reject-condition' 'struct S{int x;};int main(void){struct S s;if(s)return 1;return 0;}' 0 $true
    Check 'record-reject-distinct-assignment' 'struct S{int x;};struct T{int x;};int main(void){struct S s;struct T t;s=t;return 0;}' 0 $true
    Check 'record-reject-assignment-member-lvalue' 'struct S{int x;};int main(void){struct S a,b;(a=b).x=7;return 0;}' 0 $true
    Check 'record-reject-register-member-address' 'struct S{int x;};int main(void){register struct S s;int *p=&s.x;return 0;}' 0 $true
    Check 'record-reject-register-array-decay' 'struct S{int a[3];};int main(void){register struct S s;int *p=s.a;return 0;}' 0 $true
    Check 'record-value-parameter' 'struct S{int x;};int f(struct S s){return s.x;}int main(void){struct S s={7};return f(s);}' 7
    foreach ($recordBytes in 1, 2, 3, 4, 5, 7, 8, 9, 12, 16, 24, 64) {
        $last = $recordBytes - 1
        $prefix = "struct S{char a[$recordBytes];};"
        Check "aggregate-param-$recordBytes" ($prefix + "int f(struct S s){s.a[$last]=3;return s.a[$last];}int main(void){struct S s={{0}};s.a[$last]=7;return f(s)+s.a[$last];}") 10
        Check "aggregate-return-$recordBytes" ($prefix + "struct S f(int n){struct S s={{0}};s.a[$last]=n;return s;}int main(void){struct S s=f(7);return s.a[$last];}") 7
        Check "aggregate-relay-$recordBytes" ($prefix + "struct S f(struct S s,int a,int b,int c,int d){s.a[$last]+=a+b+c+d;return s;}int main(void){struct S s={{0}},t;s.a[$last]=2;t=f(s,1,2,3,4);return t.a[$last]+s.a[$last];}") 14
    }
    Check 'aggregate-nested-calls' 'struct S{int a[3];};struct S make(int n){struct S s={{n,2,3}};return s;}int sum(struct S s,struct S t){return s.a[0]+t.a[0];}int main(void){return sum(make(3),make(4));}' 7
    Check 'aggregate-recursion' 'struct S{int a[3];};struct S f(int n){struct S s={{7,0,0}};if(n==0)return s;return f(n-1);}int main(void){return f(3).a[0];}' 7
    Check 'aggregate-small-recursion' 'struct S{int n;};struct S f(int n){struct S s={7};if(n==0)return s;return f(n-1);}int main(void){return f(3).n;}' 7
    Check 'aggregate-mutual-recursion' 'struct S{int a[3];};struct S g(int);struct S f(int n){struct S s={{7}};if(n)return g(n-1);return s;}struct S g(int n){return f(n);}int main(void){return f(2).a[0];}' 7
    Check 'aggregate-union-return' 'union U{long long x;char a[8];};union U f(union U u){u.x+=3;return u;}int main(void){union U u={4};return f(u).x;}' 7
    Check 'aggregate-large-union-return' 'union U{long long x;char a[24];};union U f(union U u){u.x+=3;return u;}int main(void){union U u={4};return f(u).x;}' 7
    Check 'aggregate-copy-before-later-argument' 'struct S{int a[3];};struct S s={{3}};int change(void){s.a[0]=7;return 0;}int f(struct S x,int n){return x.a[0];}int main(void){return f(s,change())+s.a[0];}' 10
    Check 'aggregate-parameter-address' 'struct S{int a[3];};int f(struct S s){struct S *p=&s;p->a[0]=3;return s.a[0];}int main(void){struct S s={{7}};return f(s)+s.a[0];}' 10
    Check 'aggregate-parameter-scalar-mix' 'struct S{int a[3];};int f(int a,struct S s,int b,struct S t,int c,struct S u){return a+s.a[0]+b+t.a[0]+c+u.a[0];}int main(void){struct S s={{2}};return f(1,s,3,s,5,s);}' 15
    Check 'aggregate-return-register-argument' 'struct S{int a[3];};struct S f(int n){struct S s={{n}};return s;}int main(void){register int n=7;return f(n).a[0];}' 7
    Check 'aggregate-sizeof-no-call' 'struct S{int a[3];};int n;struct S f(void){struct S s={{0}};n++;return s;}int main(void){return sizeof f()+n;}' 12
    Check 'aggregate-sizeof-no-frame' 'struct S{char a[70000];};struct S f(void);int main(void){return sizeof f()==70000;}' 1
    Check 'aggregate-return-loop' 'struct S{int a[3];};struct S f(int n){struct S s={{n}};return s;}int main(void){int n=0,i;for(i=0;i<5;i++)n+=f(i).a[0];return n;}' 10
    Check 'aggregate-return-conditional' 'struct S{int a[3];};struct S f(int n){struct S a={{3}},b={{7}};return n?a:b;}int main(void){return f(0).a[0];}' 7
    Check 'aggregate-prototype-incomplete-return' 'struct S;struct S f(void);struct S{int a[3];};struct S f(void){struct S s={{7}};return s;}int main(void){return f().a[0];}' 7
    Check 'aggregate-reject-wrong-return' 'struct S{int x;};struct T{int x;};struct S f(void){struct T t={7};return t;}int main(void){return 0;}' 0 $true
    Check 'aggregate-reject-wrong-parameter' 'struct S{int x;};struct T{int x;};int f(struct S s){return s.x;}int main(void){struct T t={7};return f(t);}' 0 $true
    Check 'aggregate-reject-implicit-return-conflict' 'int main(void){f();return 0;}struct S{int x;};struct S f(void){struct S s={7};return s;}' 0 $true
    $aggregateParameters = (0..126 | ForEach-Object { "int p$_" }) -join ','
    $aggregateArguments = (0..126 | ForEach-Object { '1' }) -join ','
    Check 'aggregate-hidden-plus-127-parameters' ("struct S{int a[3];};struct S f($aggregateParameters){struct S s={{p0+p126}};return s;}int main(void){return f($aggregateArguments).a[0];}") 2
    Check 'enum-anonymous' 'enum {A,B,C};int main(void){return A+B+C;}' 3
    Check 'enum-tagged-object' 'enum E {A=3,B=7} x=B;int main(void){return x;}' 7
    Check 'enum-tag-reference' 'enum E {A=7};enum E x=A;int main(void){enum E y=x;return y;}' 7
    Check 'enum-explicit-sequence' 'enum E {A=-3,B,C=7,D};int main(void){return B+D;}' 6
    Check 'enum-constant-expression' 'enum E {A=2,B=A*3,C=B+sizeof(int)};int main(void){return C;}' 10
    Check 'enum-duplicate-values' 'enum {A=7,B=7};int main(void){return A==B;}' 1
    Check 'enum-trailing-comma' 'enum {A=7,};int main(void){return A;}' 7
    Check 'enum-int-limits' 'enum {A=-2147483647-1,B=2147483647};int main(void){return A<0 && B>0;}' 1
    Check 'enum-explicit-after-max' 'enum {A=2147483647,B=7};int main(void){return B;}' 7
    Check 'enum-sizeof' 'enum E {A};int main(void){return sizeof(enum E)+sizeof(A);}' 8
    Check 'enum-promotions' 'enum E {A=-1};int main(void){enum E x=A;return (x+1)==0 && x<0;}' 1
    Check 'enum-conversion-wide' 'enum E {A=-1};int main(void){enum E x=A;long long y=x;return y==-1LL;}' 1
    Check 'enum-conversion-bool' 'enum E {A=7};int main(void){_Bool x=A;return x;}' 1
    Check 'enum-cast' 'enum E {A};int main(void){enum E x=(enum E)7;return x;}' 7
    Check 'enum-cast-constant' 'enum E {A};enum {B=(enum E)7};int main(void){return B;}' 7
    Check 'enum-increment' 'enum E {A=2};int main(void){enum E x=A;int y=x++;return y+(++x);}' 6
    Check 'enum-compound' 'enum E {A=2};int main(void){enum E x=A;x+=5;return x;}' 7
    Check 'enum-array-bound' 'enum {N=3};int main(void){int a[N]={1,2,7};return sizeof a+a[2];}' 19
    Check 'enum-array-values' 'enum E {A=2,B=7};enum E a[]={A,B};int main(void){return sizeof a+a[1];}' 15
    Check 'enum-switch' 'enum E {A=3,B=7};int main(void){enum E x=B;switch(x){case A:return 1;case B:return 7;}return 0;}' 7
    Check 'enum-pointer' 'enum E {A=7};int main(void){enum E x=A;enum E *p=&x;return *p;}' 7
    Check 'enum-pointer-step' 'enum E {A=2,B=7};int main(void){enum E a[]={A,B};enum E *p=a;p++;return *p;}' 7
    Check 'enum-pointer-int-compatible' 'enum E {A=7};int main(void){enum E x=A;int *p=&x;return *p;}' 7
    Check 'enum-int-redeclaration' 'enum E {A=7};extern enum E x;int x=7;int main(void){return x;}' 7
    Check 'enum-function' 'enum E {A=7};enum E f(enum E x){return x;}int main(void){return f(A);}' 7
    Check 'enum-function-int-prototype' 'enum E {A=7};int f(int);enum E f(enum E x){return x;}int main(void){return f(A);}' 7
    Check 'enum-parameter-definition' 'int f(enum E {A=3,B=7} x,int y){enum E z=x;return z+y;}int main(void){return f(3,4);}' 7
    Check 'enum-parameter-array-bound' 'int f(int a[sizeof(enum E {N=7})],int y){return a[0]+y+N;}int main(void){int a[1]={2};return f(a,3);}' 12
    Check 'enum-prototype-scope' 'int f(enum E {A=7});enum {A=3};int main(void){return A;}' 3
    Check 'enum-local' 'int main(void){enum {A=7};return A;}' 7
    Check 'enum-shadow-constant' 'enum {A=2};int main(void){int r;{enum {A=7};r=A;}return r+A;}' 9
    Check 'enum-shadow-local' 'int main(void){int A=2;int r;{enum {A=7};r=A;}return r+A;}' 9
    Check 'enum-shadow-static' 'int main(void){static int A=2;int r;{enum {A=7};r=A;}return r+A;}' 9
    Check 'enum-shadow-typedef' 'typedef int A;int main(void){int r;{enum {A=7};r=A;}return r+sizeof(A);}' 11
    Check 'enum-shadow-register' 'int main(void){register int A=2;int r;{enum {A=7};r=A;}return r+A;}' 9
    Check 'enum-shadow-parameter' 'int f(int A){int r;{enum {A=7};r=A;}return r+A;}int main(void){return f(2);}' 9
    Check 'enum-shadow-tag' 'enum E {A=2};int main(void){int r;{enum E {B=7};enum E x=B;r=x;}enum E y=A;return r+y;}' 9
    Check 'enum-tag-name-space' 'enum E {E=7};int main(void){enum E x=E;return x;}' 7
    Check 'enum-label-name-space' 'enum {A=7};int main(void){goto A;return 0;A:return A;}' 7
    Check 'enum-typedef' 'typedef enum E {A=7} T;int main(void){T x=A;return x+sizeof(T);}' 11
    Check 'enum-static-local' 'int f(void){enum E {A=2};static enum E x=A;return ++x;}int main(void){f();return f();}' 4
    Check 'enum-for-scope' 'enum {A=7};int main(void){int r=0;for(enum E {B=3} x=B;x>0;x--)r++;return r+A;}' 10
    Check 'enum-sizeof-definition' 'int main(void){int n=sizeof(enum E {A=7});enum E x=A;return n+x;}' 11
    Check 'enum-null-pointer' 'enum {Z=0};int main(void){int *p=Z;return p==Z;}' 1
    Check 'enum-incomplete-pointer-sizeof' 'enum E {A=sizeof(enum E*)};int main(void){return A;}' 8
    Check 'enum-reject-empty' 'enum E {};int main(void){return 0;}' 0 $true
    Check 'enum-reject-duplicate-constant' 'enum {A,A};int main(void){return 0;}' 0 $true
    Check 'enum-reject-redefinition' 'enum E {A};enum E {B};int main(void){return 0;}' 0 $true
    Check 'enum-reject-forward-tag' 'enum E;int main(void){return 0;}' 0 $true
    Check 'enum-reject-implicit-overflow' 'enum {A=2147483647,B};int main(void){return 0;}' 0 $true
    Check 'enum-reject-unsigned-overflow' 'enum {A=0xffffffffU};int main(void){return 0;}' 0 $true
    Check 'enum-reject-wide-overflow' 'enum {A=0xffffffffffffffffULL};int main(void){return 0;}' 0 $true
    Check 'enum-reject-incomplete-sizeof' 'enum E {A=sizeof(enum E)};int main(void){return 0;}' 0 $true
    Check 'enum-reject-nonconstant' 'int x;enum {A=x};int main(void){return 0;}' 0 $true
    Check 'enum-reject-distinct-type' 'enum E {A};enum F {B};extern enum E x;enum F x;int main(void){return 0;}' 0 $true
    Check 'enum-reject-address' 'enum {A};int main(void){int *p=&A;return 0;}' 0 $true
    Check 'enum-reject-assignment' 'enum {A};int main(void){A=7;return 0;}' 0 $true
    Check 'enum-reject-scope-leak' 'int main(void){{enum {A=7};}return A;}' 0 $true
    Check 'enum-reject-tag-scope-leak' 'int f(void){enum E {A};return 0;}enum E x;int main(void){return 0;}' 0 $true
    Check 'enum-reject-global-collision' 'enum {A};int A;int main(void){return 0;}' 0 $true
    Check 'compatible-array-pointer-init' 'int main(void){int a[3]={1,2,7};int (*p)[]=&a;return (*p)[2];}' 7
    Check 'compatible-array-pointer-assign' 'int main(void){int a[3]={1,2,7};int (*p)[];p=&a;return (*p)[2];}' 7
    Check 'compatible-array-pointer-reverse' 'int main(void){int a[3]={1,2,7};int (*p)[]=&a;int (*q)[3]=p;return (*q)[2];}' 7
    Check 'compatible-array-pointer-global' 'int a[3]={1,2,7};int (*p)[]=&a;int main(void){return (*p)[2];}' 7
    Check 'compatible-array-pointer-static' 'int a[3]={1,2,7};int main(void){static int (*p)[]=&a;return (*p)[2];}' 7
    Check 'compatible-array-pointer-argument' 'int f(int (*p)[]){return (*p)[2];}int main(void){int a[3]={1,2,7};return f(&a);}' 7
    Check 'compatible-array-pointer-argument-reverse' 'int f(int (*p)[3]){return (*p)[2];}int main(void){int a[3]={1,2,7};int (*p)[]=&a;return f(p);}' 7
    Check 'compatible-array-pointer-prototype' 'int f(int (*)[]);int f(int (*p)[3]){return sizeof *p;}int main(void){int a[3];return f(&a);}' 12
    Check 'compatible-array-pointer-prototype-reverse' 'int f(int (*)[3]);int f(int (*p)[]){return (*p)[2];}int main(void){int a[3]={1,2,7};return f(&a);}' 7
    Check 'compatible-array-pointer-return' 'typedef int (*P)[];int a[3]={1,2,7};P f(void){return &a;}int main(void){return (*f())[2];}' 7
    Check 'compatible-array-pointer-return-prototype' 'typedef int (*P)[];typedef int (*Q)[3];P f(void);int a[3];Q f(void){return &a;}int main(void){return sizeof *f();}' 12
    Check 'compatible-array-pointer-conditional' 'int main(void){int a[3];int (*p)[]=&a;return sizeof *(1?p:&a);}' 12
    Check 'compatible-array-pointer-conditional-reverse' 'int main(void){int a[3];int (*p)[]=&a;return sizeof *(0?&a:p);}' 12
    Check 'compatible-array-pointer-conditional-value' 'int main(void){int a[3]={1,2,7};int (*p)[]=&a;return (*(0?&a:p))[2];}' 7
    Check 'compatible-array-pointer-equality' 'int main(void){int a[3];int (*p)[]=&a;return p==&a;}' 1
    Check 'compatible-array-pointer-inequality' 'int main(void){int a[3];int (*p)[]=&a;return p!=&a;}' 0
    Check 'compatible-array-pointer-relational' 'int main(void){int a[3];int (*p)[]=&a;return p<=&a && &a>=p;}' 1
    Check 'compatible-array-pointer-nested' 'int main(void){int a[3]={1,2,7};int (*p)[3]=&a;int (**q)[]=&p;return (**q)[2];}' 7
    Check 'compatible-array-pointer-void' 'int main(void){int a[3];int (*p)[]=&a;void *q=p;return q==&a;}' 1
    Check 'compatible-array-pointer-array-init' 'int a[3]={1,2,7};int (*p[])[]={&a};int main(void){return (*p[0])[2];}' 7
    Check 'compatible-array-pointer-no-type-mutation' 'int main(void){int a[3];int (*p)[]=&a;return sizeof *p;}' 0 $true
    Check 'compatible-array-pointer-conditional-incomplete' 'int main(void){int (*p)[]=0;int (*q)[]=0;return sizeof *(1?p:q);}' 0 $true
    Check 'compatible-array-pointer-bad-length' 'int main(void){int a[3];int (*p)[4]=&a;return 0;}' 0 $true
    Check 'compatible-array-pointer-bad-element' 'int main(void){int a[3];long (*p)[]=&a;return 0;}' 0 $true
    Check 'compatible-array-pointer-bad-argument' 'int f(int (*p)[4]){return 0;}int main(void){int a[3];return f(&a);}' 0 $true
    Check 'compatible-array-pointer-bad-prototype' 'int f(int (*)[3]);int f(int (*p)[4]){return 0;}int main(void){return 0;}' 0 $true
    Check 'compatible-array-pointer-bad-conditional' 'int main(void){int a[3],b[4];return (1?&a:&b)!=0;}' 0 $true
    Check 'compatible-array-pointer-incomplete-add' 'int main(void){int (*p)[]=0;p=p+1;return 0;}' 0 $true
    Check 'compatible-array-pointer-incomplete-difference' 'int main(void){int a[3];int (*p)[]=&a;return &a-p;}' 0 $true
    Check 'compatible-array-pointer-nested-void-invalid' 'int main(void){int *p=0;void **q=&p;return 0;}' 0 $true
    Check 'infer-local-int' 'int main(void){int a[]={1,2,7};return sizeof a+a[2];}' 19
    Check 'infer-global-int' 'int a[]={1,2,7};int main(void){return sizeof a+a[2];}' 19
    Check 'infer-static-local' 'int f(void){static int a[]={1,2};return ++a[1];}int main(void){f();return f();}' 4
    Check 'infer-static-file' 'static int a[]={2,3};int main(void){return sizeof a+a[1];}' 11
    Check 'infer-extern-definition' 'extern int a[]={2,3};int main(void){return sizeof a;}' 8
    Check 'infer-after-extern' 'extern int a[];int f(void){return a[1];}int a[]={2,7};int main(void){return f();}' 7
    Check 'infer-after-tentative' 'int a[];int a[]={2,7};int main(void){return sizeof a+a[1];}' 15
    Check 'infer-char-list' 'int main(void){char a[]={65,66,0};return sizeof a+a[1];}' 69
    Check 'infer-wide-list' 'unsigned long long a[]={0xffffffffffffffffULL,7};int main(void){return sizeof a+a[1];}' 23
    Check 'infer-pointer-list' 'int n=7;int *a[]={&n,0};int main(void){return sizeof a+*a[0];}' 23
    Check 'infer-string-pointers' 'char *a[]={"ab","cd"};int main(void){return sizeof a+a[1][1];}' 116
    Check 'infer-local-side-effects' 'int n;int f(void){return ++n;}int main(void){int a[]={f(),f(),f()};return n*10+a[2];}' 33
    Check 'infer-comma-expression' 'int main(void){int n=0;int a[]={(n++,7),3};return sizeof a+n+a[0];}' 16
    Check 'infer-conditional-comma' 'int main(void){int a[]={1?2,3:4,7};return sizeof a+a[0];}' 11
    Check 'infer-nested-conditional' 'int main(void){int a[]={1?(0?2:3),4:5,7};return sizeof a+a[0];}' 12
    Check 'infer-call-commas' 'int f(int a,int b){return a+b;}int main(void){int a[]={f(2,3),7};return sizeof a+a[0];}' 13
    Check 'infer-sizeof-type' 'int main(void){int a[]={sizeof(int[2]),7};return sizeof a+a[0];}' 16
    Check 'infer-self-address-sizeof' 'int main(void){int a[]={sizeof &a,7};return sizeof a+a[0];}' 16
    Check 'infer-matrix-braces' 'int a[][2]={{1,2},{7}};int main(void){return sizeof a+a[1][0]+a[1][1];}' 23
    Check 'infer-matrix-flat' 'int main(void){int a[][2]={1,2,3};return sizeof a+a[1][0]+a[1][1];}' 19
    Check 'infer-matrix-mixed' 'int main(void){int a[][2]={{1},2,3,{7}};return sizeof a+a[2][0]+a[2][1];}' 31
    Check 'infer-cube-flat' 'int a[][2][2]={1,2,3,4,5};int main(void){return sizeof a+a[1][0][0]+a[1][1][1];}' 37
    Check 'infer-string-rows' 'char a[][4]={"a","bc"};int main(void){return sizeof a+a[1][1]+a[0][3];}' 107
    Check 'infer-string-rows-braces' 'int main(void){char a[][4]={{"a"},{"bc"}};return sizeof a+a[1][1];}' 107
    Check 'infer-typedef' 'typedef int A[];int main(void){A a={1,2},b={3,4,5};return sizeof a+sizeof b;}' 20
    Check 'infer-register' 'int main(void){register int a[]={1,2,3};return sizeof a;}' 12
    Check 'infer-for' 'int main(void){for(int a[]={1,7};a[0];a[0]=0){return sizeof a+a[1];}return 0;}' 15
    Check 'infer-trailing-comma' 'int a[]={1,2,};int main(void){return sizeof a;}' 8
    Check 'infer-scalar-braces' 'int a[]={{1},{{2}}};int main(void){return sizeof a+a[1];}' 10
    Check 'infer-completed-next-declarator' 'int main(void){int a[]={1,2},b[sizeof a];return sizeof b;}' 32
    Check 'infer-self-sizeof-invalid' 'int main(void){int a[]={sizeof a};return 0;}' 0 $true
    Check 'infer-global-self-sizeof-invalid' 'int a[]={sizeof a};int main(void){return 0;}' 0 $true
    Check 'infer-static-self-sizeof-invalid' 'int main(void){static int a[]={sizeof a};return 0;}' 0 $true
    Check 'infer-typedef-remains-incomplete' 'typedef int A[];int main(void){A a={1,2};return sizeof(A);}' 0 $true
    Check 'infer-empty-invalid' 'int a[]={};int main(void){return 0;}' 0 $true
    Check 'infer-no-braces-invalid' 'int a[]=7;int main(void){return 0;}' 0 $true
    Check 'infer-missing-element-invalid' 'int a[]={1,,2};int main(void){return 0;}' 0 $true
    Check 'infer-missing-close-invalid' 'int a[]={1,2;int main(void){return 0;}' 0 $true
    Check 'infer-incompatible-prior-bound' 'extern int a[1];int a[]={1,2};int main(void){return 0;}' 0 $true
    Check 'infer-too-many-row-elements' 'int a[][2]={{1,2,3}};int main(void){return 0;}' 0 $true
    Check 'extern-later-definition' 'extern int n;int main(void){return n;}int n=7;' 7
    Check 'extern-later-store' 'extern int n;int main(void){n=7;return n;}int n;' 7
    Check 'extern-address-fixup' 'extern int n;int main(void){int *p=&n;return *p;}int n=8;' 8
    Check 'extern-initializer-fixup' 'extern int n;int *p=&n;int main(void){return *p;}int n=9;' 9
    Check 'extern-array-fixup' 'extern int a[];int main(void){return a[2];}int a[3]={1,2,7};' 7
    Check 'extern-array-pointer-fixup' 'extern int a[];int *p=a;int main(void){return p[2];}int a[3]={1,2,8};' 8
    Check 'extern-array-address-fixup' 'extern int a[];int main(void){return (*&a)[1];}int a[2]={1,7};' 7
    Check 'extern-unused' 'extern int n;int main(void){return 3;}' 3
    Check 'extern-sizeof-no-definition' 'extern int n;int main(void){return sizeof n;}' 4
    Check 'extern-sizeof-address-no-definition' 'extern int n;int main(void){return sizeof(&n);}' 8
    Check 'extern-sizeof-mutation-no-definition' 'extern int n;int main(void){return sizeof(n++);}' 4
    Check 'extern-sizeof-array-no-definition' 'extern int a[3];int main(void){return sizeof a;}' 12
    Check 'extern-sizeof-nested' 'extern int a[3];int main(void){return sizeof(sizeof a);}' 8
    Check 'extern-initialized-definition' 'extern int n=7;int main(void){return n;}' 7
    Check 'extern-repeated' 'extern int n;extern int n;int n=7;extern int n;int main(void){return n;}' 7
    Check 'extern-function' 'extern int f(void);int main(void){return f();}int f(void){return 7;}' 7
    Check 'extern-static-function' 'static int f(void);extern int f(void);int f(void){return 7;}int main(void){return f();}' 7
    Check 'extern-static-object' 'static int n=7;extern int n;int main(void){return n;}' 7
    Check 'extern-static-local-initializer' 'extern int n;int f(void){static int *p=&n;return *p;}int main(void){return f();}int n=7;' 7
    Check 'tentative-before-definition' 'int n;int n=7;int main(void){return n;}' 7
    Check 'tentative-after-definition' 'int n=7;int n;int n;int main(void){return n;}' 7
    Check 'tentative-array-completion' 'int a[];int a[3];int main(void){return sizeof a;}' 12
    Check 'tentative-array-final-one' 'int a[];int main(void){a[0]=7;return a[0];}' 7
    Check 'tentative-array-address' 'int a[];int *p=a;int main(void){*p=8;return a[0];}' 8
    Check 'tentative-array-repeated' 'int a[3];int a[];int a[3];int main(void){return sizeof a;}' 12
    Check 'tentative-matrix-completion' 'extern int a[][3];int a[2][3];int main(void){return sizeof a;}' 24
    Check 'tentative-pointer-composite' 'int (*p)[];int (*p)[3];int main(void){int a[1][3];p=a;return sizeof *p;}' 12
    Check 'tentative-static-repeated' 'static int n;static int n;static int n=7;int main(void){return n;}' 7
    Check 'extern-unresolved-used' 'extern int n;int main(void){return n;}' 0 $true
    Check 'extern-unresolved-address' 'extern int n;int *p=&n;int main(void){return p!=0;}' 0 $true
    Check 'extern-incompatible-type' 'extern int n;long n;int main(void){return 0;}' 0 $true
    Check 'extern-incompatible-array' 'extern int a[3];int a[4];int main(void){return 0;}' 0 $true
    Check 'extern-object-function-conflict' 'extern int f;int f(void){return 0;}int main(void){return 0;}' 0 $true
    Check 'extern-object-called' 'extern int f;int main(void){return f();}' 0 $true
    Check 'extern-block-unused' 'int main(void){extern int n;return 0;}' 0
    Check 'extern-block-later-definition' 'int main(void){extern int n;return n;}int n=7;' 7
    Check 'extern-block-store' 'int main(void){extern int n;n=8;return n;}int n;' 8
    Check 'extern-block-address' 'int main(void){extern int n;int *p=&n;return *p;}int n=9;' 9
    Check 'extern-block-static-initializer' 'int main(void){extern int n;static int *p=&n;return *p;}int n=7;' 7
    Check 'extern-block-shadow-auto' 'int n=7;int main(void){int n=2;int r;{extern int n;r=n;}return r+n;}' 9
    Check 'extern-block-shadow-static-local' 'int n=7;int main(void){static int n=2;int r;{extern int n;r=n;}return r+n;}' 9
    Check 'extern-block-shadow-register' 'int n=7;int main(void){register int n=2;int r;{extern int n;int *p=&n;r=*p;}return r+n;}' 9
    Check 'extern-block-shadow-parameter' 'int n=7;int f(int n){int r;{extern int n;r=n;}return r+n;}int main(void){return f(2);}' 9
    Check 'extern-block-shadow-typedef' 'int main(void){typedef int n;int r;{extern long n;r=sizeof n;}return r+sizeof(n);}' 8
    Check 'extern-block-file-typedef' 'typedef int n;int main(void){{extern long long n;if(sizeof n!=8)return 1;}return sizeof(n);}' 4
    Check 'extern-block-later-typedef' 'int f(void){extern long long n;return sizeof n;}typedef int n;int main(void){return f()+sizeof(n);}' 12
    Check 'extern-block-inherit-static' 'static int n=7;int main(void){extern int n;return n;}' 7
    Check 'extern-block-repeat' 'int main(void){extern int n;extern int n;return n;}int n=7;' 7
    Check 'extern-block-multiple' 'int main(void){extern int a,b;return a+b;}int a=3,b=4;' 7
    Check 'extern-block-across-functions' 'int f(void){extern int n;return n;}int main(void){extern int n;return f()+n;}int n=4;' 8
    Check 'extern-block-array' 'int main(void){extern int a[];return a[1];}int a[2]={3,7};' 7
    Check 'extern-block-array-composite' 'extern int a[];int main(void){extern int a[3];extern int a[];return sizeof a;}' 12
    Check 'extern-block-array-inherit' 'int a[3];int main(void){extern int a[];return sizeof a;}' 12
    Check 'extern-block-array-nested-restore' 'int main(void){extern int a[];{extern int a[3];if(sizeof a!=12)return 1;}return a[1];}int a[2]={1,7};' 0 $true
    Check 'extern-block-sizeof-undefined' 'int main(void){extern long long n;return sizeof(n++);}' 8
    Check 'extern-block-no-leak' 'int main(void){{extern int n;}return n;}int n=7;' 0 $true
    Check 'extern-block-no-function-leak' 'int f(void){extern int n;return sizeof n;}int main(void){return n;}int n=7;' 0 $true
    Check 'extern-block-no-type-leak' 'extern int a[];int f(void){extern int a[3];return sizeof a;}int main(void){return sizeof a;}' 0 $true
    Check 'extern-block-no-nested-type-leak' 'int main(void){extern int a[];{extern int a[3];}return sizeof a;}' 0 $true
    Check 'extern-block-initializer-invalid' 'int main(void){extern int n=7;return n;}' 0 $true
    Check 'extern-block-for-invalid' 'int n;int main(void){for(extern int n;n<1;n++){}return 0;}' 0 $true
    Check 'extern-block-auto-conflict' 'int main(void){int n;extern int n;return 0;}' 0 $true
    Check 'extern-block-reverse-auto-conflict' 'int main(void){extern int n;int n;return 0;}' 0 $true
    Check 'extern-block-type-conflict' 'long n;int main(void){extern int n;return 0;}' 0 $true
    Check 'extern-block-cross-type-conflict' 'int f(void){extern long n;return 0;}int main(void){extern int n;return 0;}' 0 $true
    Check 'extern-block-hidden-static-conflict' 'static int n;int main(void){int n;{extern int n;}return 0;}' 0 $true
    Check 'extern-block-unresolved' 'int main(void){extern int n;return n;}' 0 $true
    Check 'extern-parameter-invalid' 'int f(extern int n){return n;}int main(void){return 0;}' 0 $true
    Check 'tentative-duplicate-initializer' 'int n=1;int n=2;int main(void){return 0;}' 0 $true
    Check 'tentative-incomplete-sizeof' 'int a[];int main(void){return sizeof a;}' 0 $true
    Check 'tentative-static-incomplete' 'static int a[];int main(void){return 0;}' 0 $true
    Check 'tentative-static-linkage-conflict' 'static int n;int n;int main(void){return 0;}' 0 $true
    Check 'static-local-zero' 'int f(void){static int n;return ++n;}int main(void){int a=f();return a*10+f();}' 12
    Check 'static-local-initialized' 'int f(void){static int n=5;return ++n;}int main(void){int a=f();return a*10+f();}' 67
    Check 'static-local-array' 'int f(void){static int a[2]={1,2};return ++a[1];}int main(void){int a=f();return a*10+f();}' 34
    Check 'static-local-string' 'int f(void){static char a[]="ab";a[0]++;return a[0];}int main(void){int a=f();return f()-a;}' 1
    Check 'static-local-wide' 'long long f(void){static long long n=1LL<<40;return n++;}int main(void){long long n=f();return f()-n;}' 1
    Check 'static-local-address-lifetime' 'int *f(void){static int n=7;return &n;}int main(void){int *p=f();*p=9;return *f();}' 9
    Check 'static-local-pointer-initializer' 'int f(void){static int n=7;static int *p=&n;return ++*p;}int main(void){int n=f();return n*10+f();}' 89
    Check 'static-local-self-address' 'int f(void){static void *p=&p;return p==&p;}int main(void){return f();}' 1
    Check 'static-loop' 'int main(void){int n=0;for(int i=0;i<3;i++){static int x=2;n+=x++;}return n;}' 9
    Check 'static-recursion' 'int f(int n){static int x;if(n==0)return x;x++;return f(n-1);}int main(void){return f(5);}' 5
    Check 'static-separated-functions' 'int f(void){static int n=1;return n++;}int g(void){static int n=7;return n++;}int main(void){int a=f();return a*10+g();}' 17
    Check 'static-shadow-global' 'int n=3;int f(void){static int n=7;return n;}int main(void){return f()+n;}' 10
    Check 'static-shadow-local' 'int main(void){int n=3;int r=0;{static int n=7;r=n;}return n+r;}' 10
    Check 'static-shadow-static' 'int main(void){static int n=3;int r=0;{static int n=7;r=n;}return n+r;}' 10
    Check 'static-shadow-typedef' 'typedef int I;int f(void){static int I=7;return I;}int main(void){I n=3;return f()+n;}' 10
    Check 'static-multiple' 'int f(void){static int a=2,b=3;return a+++b++;}int main(void){int n=f();return n*10+f();}' 57
    Check 'static-large-object' 'int f(void){static char a[100000];a[99999]++;return a[99999];}int main(void){int n=f();return n*10+f();}' 12
    Check 'static-file-object' 'static int n=7;int main(void){return n;}' 7
    Check 'static-file-function' 'static int f(void){return 7;}int main(void){return f();}' 7
    Check 'static-function-prototype' 'static int f(void);int main(void){return f();}static int f(void){return 9;}' 9
    Check 'static-function-inherit-linkage' 'static int f(void);int f(void){return 8;}int main(void){return f();}' 8
    Check 'static-init-chain' 'int a=1;int f(void){static int b=2;return b;}int c=3;int g(void){static int d=4;return d;}int main(void){return a+f()+c+g();}' 10
    Check 'static-sizeof' 'int f(void){static int a[3];return sizeof a;}int main(void){return f();}' 12
    Check 'static-runtime-initializer' 'int f(int n){static int x=n;return x;}int main(void){return f(1);}' 0 $true
    Check 'static-auto-address-initializer' 'int f(void){int n;static int *p=&n;return 0;}int main(void){return f();}' 0 $true
    Check 'static-shadowed-global-address' 'int n;int f(void){int n;static int *p=&n;return 0;}int main(void){return f();}' 0 $true
    Check 'static-parameter-rejected' 'int f(static int n){return n;}int main(void){return 0;}' 0 $true
    Check 'static-for-rejected' 'int main(void){for(static int n=0;n<2;n++){}return 0;}' 0 $true
    Check 'static-duplicate-local' 'int main(void){static int n;static int n;return 0;}' 0 $true
    Check 'static-conflicting-linkage' 'int f(void);static int f(void){return 0;}int main(void){return 0;}' 0 $true
    Check 'storage-auto-local' 'int main(void){auto int n=7;return n;}' 7
    Check 'storage-register-local' 'int main(void){register int n=7;n+=2;return n;}' 9
    Check 'storage-position' 'int main(void){int register n=4;int auto m=3;return n+m;}' 7
    Check 'storage-auto-address' 'int main(void){auto int n=7;return *&n;}' 7
    Check 'storage-register-pointer' 'int main(void){int n=7;register int *p=&n;return *p;}' 7
    Check 'storage-register-pointee-address' 'int main(void){int n=7;register int *p=&n;return *&*p;}' 7
    Check 'storage-register-parameter' 'int f(register int n){n++;return n;}int main(void){return f(7);}' 8
    Check 'storage-register-stack-parameter' 'int f(int a,int b,int c,int d,register long long e){return e>>32;}int main(void){return f(1,2,3,4,1LL<<32);}' 1
    Check 'storage-register-prototype' 'int f(register int);int f(int n){return *&n;}int main(void){return f(7);}' 7
    Check 'storage-register-array-parameter' 'int f(register int a[]){return a[1];}int main(void){int a[2]={1,7};return f(a);}' 7
    Check 'storage-register-sizeof' 'int main(void){register int n=7;return sizeof(n)+sizeof(n++);}' 8
    Check 'storage-register-array-sizeof' 'int main(void){register int a[3]={1,2,3};return sizeof a;}' 12
    Check 'storage-register-for' 'int main(void){int n=0;for(register int i=0;i<4;i++)n+=i;return n;}' 6
    Check 'storage-register-typedef' 'typedef int I;int main(void){register I n=7;return n;}' 7
    Check 'storage-register-shadow' 'int main(void){register int n=7;{int n=3;int *p=&n;*p=4;}return n;}' 7
    Check 'storage-register-scope-restore' 'int main(void){int n=7;{register int n=3;}return *&n;}' 7
    Check 'storage-register-alias-shadow' 'int main(void){register int n=7;{typedef short n;n x=3;}return n;}' 7
    Check 'storage-register-multiple' 'int main(void){register int n=3,m=4;return n+m;}' 7
    Check 'storage-register-address' 'int main(void){register int n=7;return *&n;}' 0 $true
    Check 'storage-register-address-parentheses' 'int main(void){register int n=7;return *&(n);}' 0 $true
    Check 'storage-register-address-parameter' 'int f(register int n){return *&n;}int main(void){return f(7);}' 0 $true
    Check 'storage-register-restored-address' 'int main(void){register int n=7;{int n=2;}return *&n;}' 0 $true
    Check 'storage-register-array-decay' 'int main(void){register int a[2];int *p=a;return 0;}' 0 $true
    Check 'storage-register-array-address' 'int main(void){register int a[2];return sizeof(&a);}' 0 $true
    Check 'storage-auto-parameter' 'int f(auto int n){return n;}int main(void){return 0;}' 0 $true
    Check 'storage-auto-global' 'auto int n;int main(void){return 0;}' 0 $true
    Check 'storage-register-global' 'register int n;int main(void){return 0;}' 0 $true
    Check 'storage-register-function' 'register int f(void){return 0;}int main(void){return 0;}' 0 $true
    Check 'storage-duplicate' 'int main(void){register register int n;return 0;}' 0 $true
    Check 'storage-conflict' 'int main(void){auto register int n;return 0;}' 0 $true
    Check 'storage-typedef-conflict' 'typedef register int I;int main(void){return 0;}' 0 $true
    Check 'storage-cast' 'int main(void){return (register int)0;}' 0 $true
    Check 'bool-zero' 'int main(void){_Bool b=0;return b;}' 0
    Check 'bool-nonzero' 'int main(void){_Bool b=257;return b;}' 1
    Check 'bool-negative' 'int main(void){_Bool b=-7;return b;}' 1
    Check 'bool-wide' 'int main(void){_Bool b=1ULL<<40;return b;}' 1
    Check 'bool-pointer' 'int main(void){int x;_Bool b=&x;return b;}' 1
    Check 'bool-null-pointer' 'int main(void){int *p=0;_Bool b=p;return b;}' 0
    Check 'bool-high-pointer' 'int main(void){return (_Bool)(void*)(1ULL<<40);}' 1
    Check 'bool-sizeof' 'int main(void){_Bool b;return sizeof b+sizeof(_Bool);}' 2
    Check 'bool-array' 'int main(void){_Bool a[3]={0,7,256};return sizeof a+a[0]+a[1]+a[2];}' 5
    Check 'bool-global' '_Bool b=-9;int main(void){return b;}' 1
    Check 'bool-global-array' '_Bool a[3]={0,7,256};int main(void){return a[0]+a[1]+a[2];}' 2
    Check 'bool-address-store' 'int main(void){_Bool b=0;_Bool *p=&b;*p=256;return b;}' 1
    Check 'bool-pointer-step' 'int main(void){_Bool a[3]={0,1,0};_Bool *p=a;p++;return *p+(p-a);}' 2
    Check 'bool-promotion' 'int main(void){_Bool a=1,b=1;return a+b;}' 2
    Check 'bool-negation' 'int main(void){_Bool b=1;return -b;}' -1
    Check 'bool-assignment-result' 'int main(void){_Bool b=0;return b=256;}' 1
    Check 'bool-postincrement' 'int main(void){_Bool b=1;int old=b++;return old*10+b;}' 11
    Check 'bool-preincrement' 'int main(void){_Bool b=0;return ++b;}' 1
    Check 'bool-decrement-true' 'int main(void){_Bool b=1;b--;return b;}' 0
    Check 'bool-decrement-false' 'int main(void){_Bool b=0;b--;return b;}' 1
    Check 'bool-compound-wide' 'int main(void){_Bool b=0;b+=1ULL<<40;return b;}' 1
    Check 'bool-compound-subtract' 'int main(void){_Bool b=1;b-=1;return b;}' 0
    Check 'bool-parameter-return' '_Bool f(_Bool b){return b+3;}int main(void){return f(256);}' 1
    Check 'bool-stack-parameter' 'int f(int a,int b,int c,int d,_Bool e){return e;}int main(void){return f(1,2,3,4,256);}' 1
    Check 'bool-typedef' 'typedef _Bool B;int main(void){B b=7;return b;}' 1
    Check 'bool-constant-cast' 'int n=(_Bool)256;int main(void){return n;}' 1
    Check 'bool-bound' 'int main(void){char a[(_Bool)256];return sizeof a;}' 1
    Check 'bool-conditional' 'int main(void){_Bool b=1;return (1?b:2)+1;}' 2
    Check 'bool-switch' 'int main(void){_Bool b=8;switch(b){case 1:return 7;case 2:return 9;}return 0;}' 7
    Check 'bool-sizeof-increment' 'int main(void){_Bool b=0;int n=sizeof(b++);return n+b;}' 1
    Check 'bool-unsigned-invalid' 'unsigned _Bool b;int main(void){return 0;}' 0 $true
    Check 'bool-duplicate-invalid' '_Bool _Bool b;int main(void){return 0;}' 0 $true
    Check 'bool-int-invalid' 'int _Bool b;int main(void){return 0;}' 0 $true
    Check 'bool-void-invalid' 'int main(void){_Bool b=(void)0;return b;}' 0 $true
    Check 'typedef-global' 'typedef int I;I x=7;int main(void){return x;}' 7
    Check 'typedef-local' 'int main(void){typedef unsigned char C;C c=257;return c;}' 1
    Check 'typedef-chain' 'typedef unsigned long long U;typedef U V;int main(void){V n=1ULL<<40;return n>>40;}' 1
    Check 'typedef-multiple' 'typedef int I,*P,A[3];int main(void){A a={1,2,3};P p=a;I n=p[2];return n+sizeof a;}' 15
    Check 'typedef-pointer-array' 'typedef int Row[3];typedef Row *P;int main(void){Row a[2];P p=a;p[1][2]=8;return a[1][2];}' 8
    Check 'typedef-array-pointer' 'typedef int (*P)[3];int main(void){int a[2][3];P p=a;p[1][2]=9;return a[1][2];}' 9
    Check 'typedef-parameter-return' 'typedef long long L;L f(L a){return a+1;}int main(void){return f(4294967296LL)==4294967297LL;}' 1
    Check 'typedef-array-parameter' 'typedef int A[3];int f(A a){return a[2];}int main(void){A a={1,2,7};return f(a);}' 7
    Check 'typedef-void' 'typedef void V;V f(void){return;}int main(void){f();return 4;}' 4
    Check 'typedef-void-pointer' 'typedef void V;int main(void){int x=7;V *p=&x;return *(int*)p;}' 7
    Check 'typedef-sizeof' 'typedef short S;typedef S A[3];int main(void){return sizeof(A);}' 6
    Check 'typedef-cast' 'typedef unsigned char C;int main(void){return (C)258;}' 2
    Check 'typedef-bound' 'typedef short S;int a[sizeof(S)+1];int main(void){return sizeof a;}' 12
    Check 'typedef-nested-bound' 'typedef int A[sizeof(short[2])];int main(void){return sizeof(A);}' 16
    Check 'typedef-constant-cast' 'typedef unsigned char C;int n=(C)258;int main(void){return n;}' 2
    Check 'typedef-shadow-variable' 'typedef int T;int main(void){int n=0;{int T=7;n=T;}T x=2;return n+x;}' 9
    Check 'typedef-shadow-alias' 'typedef int T;int main(void){int n=0;{typedef char T;T x=7;n=sizeof x;}return n+sizeof(T);}' 5
    Check 'typedef-shadow-object' 'int main(void){int T=7;{typedef char T;T x=2;}return T;}' 7
    Check 'typedef-shadow-parameter' 'typedef int T;int f(int T){return T;}int main(void){T x=3;return f(x);}' 3
    Check 'typedef-prototype-scope-restore' 'typedef int T;int f(int T);T x=3;int f(int n){return n;}int main(void){return f(x);}' 3
    Check 'typedef-mixed-prototype-restore' 'typedef int T;int f(int T,int);T x=3;int f(int n,int m){return n+m;}int main(void){return f(x,4);}' 7
    Check 'typedef-same-type-name-parameter' 'typedef int T;int f(T T){return T;}int main(void){return f(7);}' 7
    Check 'typedef-for-variable' 'typedef int I;int main(void){int n=0;for(I i=0;i<4;i++)n+=i;return n;}' 6
    Check 'typedef-label-namespace' 'typedef int T;int main(void){goto T;T:return sizeof(T);}' 4
    Check 'typedef-storage-position' 'int typedef I;int main(void){I x=4;return x;}' 4
    Check 'typedef-function-scope-restore' 'typedef int T;int f(void){typedef char T;return sizeof(T);}int main(void){return f()+sizeof(T);}' 5
    Check 'typedef-expression-rejected' 'typedef int T;int main(void){return T;}' 0 $true
    Check 'typedef-initializer-rejected' 'typedef int T=1;int main(void){return 0;}' 0 $true
    Check 'typedef-global-object-conflict' 'typedef int T;int T;int main(void){return 0;}' 0 $true
    Check 'typedef-local-object-conflict' 'int main(void){typedef int T;int T;return 0;}' 0 $true
    Check 'typedef-object-local-conflict' 'int main(void){int T;typedef int T;return 0;}' 0 $true
    Check 'typedef-global-function-conflict' 'typedef int T;int T(void){return 0;}int main(void){return 0;}' 0 $true
    Check 'typedef-parameter-storage-rejected' 'int f(typedef int T){return 0;}int main(void){return 0;}' 0 $true
    Check 'typedef-cast-storage-rejected' 'int main(void){return (typedef int)0;}' 0 $true
    Check 'typedef-for-storage-rejected' 'int main(void){for(typedef int T;;)break;return 0;}' 0 $true
    Check 'typedef-duplicate-specifier' 'typedef typedef int T;int main(void){return 0;}' 0 $true
    Check 'typedef-out-of-scope' 'int main(void){{typedef int T;}T x;return 0;}' 0 $true
    Check 'bound-local-arithmetic' 'int main(void){int a[2+3];a[4]=7;return sizeof a+a[4];}' 27
    Check 'bound-global-arithmetic' 'int a[2*3+1];int main(void){return sizeof a;}' 28
    Check 'bound-parentheses' 'int main(void){int a[(2+3)*2];return sizeof a;}' 40
    Check 'bound-sizeof-type' 'int main(void){char a[sizeof(int)+sizeof(long long)];return sizeof a;}' 12
    Check 'bound-sizeof-array-type' 'int main(void){int a[2][sizeof(int[3])];return sizeof a;}' 96
    Check 'bound-sizeof-object' 'int a[3];int b[sizeof a/sizeof a[0]];int main(void){return sizeof b;}' 12
    Check 'bound-sizeof-nested' 'int main(void){char a[sizeof(int[sizeof(short)])];return sizeof a;}' 8
    Check 'bound-cast' 'int main(void){int a[(unsigned char)258];return sizeof a;}' 8
    Check 'bound-matrix-cast' 'int main(void){int a[3][(int)2];a[2][1]=7;return sizeof a+a[2][1];}' 31
    Check 'bound-pointer-array' 'int main(void){int *a[sizeof(short)][2];return sizeof a;}' 32
    Check 'bound-array-pointer' 'int main(void){int a[2][3];int (*p)[sizeof(char[3])]=a;p[1][2]=9;return a[1][2];}' 9
    Check 'bound-abstract-pointer' 'int main(void){return sizeof(int (*)[sizeof(int[3])]);}' 8
    Check 'bound-multiple-declarators' 'int main(void){int a[2+1],b[sizeof(short)];return sizeof a+sizeof b;}' 20
    Check 'bound-parameter' 'int f(int a[sizeof(int)]){return a[3];}int main(void){int a[4]={1,2,3,9};return f(a);}' 9
    Check 'bound-parameter-matrix' 'int f(int a[][1+2]){return a[1][2];}int main(void){int a[2][3]={{1,2,3},{4,5,6}};return f(a);}' 6
    Check 'bound-conditional' 'int main(void){int a[0?9:3];return sizeof a;}' 12
    Check 'bound-short-circuit' 'int main(void){int a[1||(1/0)];return sizeof a;}' 4
    Check 'bound-sizeof-unevaluated' 'int main(void){int n=0;char a[sizeof(n++)];return sizeof a+n;}' 4
    Check 'bound-zero-expression' 'int main(void){int a[1-1];return 0;}' 0 $true
    Check 'bound-negative-expression' 'int main(void){int a[1-2];return 0;}' 0 $true
    Check 'bound-too-wide' 'int main(void){int a[1ULL<<32];return 0;}' 0 $true
    Check 'bound-overflow-expression' 'int main(void){int a[2147483647+1];return 0;}' 0 $true
    Check 'bound-runtime-unsupported' 'int main(void){int n=2;int a[n];return 0;}' 0 $true
    Check 'bound-comma-rejected' 'int main(void){int a[(1,2)];return 0;}' 0 $true
    Check 'switch-match' 'int main(void){switch(2){case 1:return 3;case 2:return 7;default:return 9;}}' 7
    Check 'switch-default' 'int main(void){switch(4){case 1:return 3;default:return 9;}}' 9
    Check 'switch-no-match' 'int main(void){int n=2;switch(4){case 1:n=9;}return n;}' 2
    Check 'switch-empty' 'int main(void){int n=2;switch(n++){}return n;}' 3
    Check 'switch-no-label' 'int main(void){int n=2;switch(4)n=7;return n;}' 2
    Check 'switch-fallthrough' 'int main(void){int n=0;switch(1){case 1:n+=2;case 2:n+=3;default:n+=4;}return n;}' 9
    Check 'switch-break' 'int main(void){int n=0;switch(1){case 1:n=3;break;default:n=7;}return n;}' 3
    Check 'switch-default-first' 'int main(void){int n=0;switch(4){default:n=2;case 1:n+=3;}return n;}' 5
    Check 'switch-consecutive-labels' 'int main(void){switch(2){case 1:case 2:case 3:return 7;}return 0;}' 7
    Check 'switch-evaluate-once' 'int main(void){int n=1;switch(n++){case 1:break;}return n;}' 2
    Check 'switch-call-controller' 'int f(void){return 2;}int main(void){switch(f()){case 2:return 8;}return 0;}' 8
    Check 'switch-negative' 'int main(void){switch(-3){case -3:return 8;}return 0;}' 8
    Check 'switch-unsigned' 'int main(void){unsigned n=4294967295U;switch(n){case -1:return 6;}return 0;}' 6
    Check 'switch-wide' 'int main(void){switch(4294967296LL){case 0:return 2;case 1LL<<32:return 7;}return 0;}' 7
    Check 'switch-wide-negative' 'int main(void){switch(-4294967296LL){case -4294967296LL:return 9;}return 0;}' 9
    Check 'switch-wide-unsigned' 'int main(void){switch(1ULL<<63){case 1ULL<<63:return 5;}return 0;}' 5
    Check 'switch-char-promotion' 'int main(void){unsigned char c=1;switch(c){case 257:return 9;case 1:return 3;}return 0;}' 3
    Check 'switch-case-expression' 'int main(void){switch(14){case 2+3*4:return 7;}return 0;}' 7
    Check 'switch-case-sizeof' 'int main(void){switch(4){case sizeof(int):return 6;}return 0;}' 6
    Check 'switch-nested' 'int main(void){int n=0;switch(1){case 1:switch(2){case 1:n=2;break;case 2:n=3;break;default:n=9;}n+=4;break;default:n=8;}return n;}' 7
    Check 'switch-loop-break' 'int main(void){int n=0;switch(1){case 1:while(1){n++;break;}n+=2;break;}return n;}' 3
    Check 'switch-loop-continue' 'int main(void){int n=0;for(int i=0;i<4;i++){switch(i){case 1:continue;default:n++;break;}n+=2;}return n;}' 9
    Check 'switch-loop-inner-break' 'int main(void){int n=0;for(int i=0;i<4;i++){switch(i){case 1:break;default:n++;}n++;}return n;}' 7
    Check 'switch-skip-initializer' 'int g;int f(void){g++;return 4;}int main(void){switch(1){int n=f();case 1:n=7;return n+g;}return 0;}' 7
    Check 'switch-label-in-if' 'int main(void){int n=0;switch(2){if(0){case 2:n=8;}}return n;}' 8
    Check 'switch-label-in-loop' 'int main(void){int n=0;switch(2){while(n<3){case 2:n++;}}return n;}' 3
    Check 'switch-goto-exit' 'int main(void){switch(1){case 1:goto done;}return 2;done:return 8;}' 8
    Check 'switch-unbraced' 'int main(void){switch(2)case 2:return 6;return 0;}' 6
    Check 'switch-default-duplicate' 'int main(void){switch(0){default:;default:;}return 0;}' 0 $true
    Check 'switch-case-duplicate' 'int main(void){switch(0){case 1:;case 2-1:;}return 0;}' 0 $true
    Check 'switch-converted-duplicate' 'int main(void){unsigned n=0;switch(n){case -1:;case 4294967295U:;}return 0;}' 0 $true
    Check 'switch-narrowed-duplicate' 'int main(void){switch(0){case 0:;case 4294967296LL:;}return 0;}' 0 $true
    Check 'switch-pointer-controller' 'int main(void){int *p=0;switch(p){default:;}return 0;}' 0 $true
    Check 'switch-case-runtime' 'int main(void){int n=1;switch(0){case n:;}return 0;}' 0 $true
    Check 'switch-case-outside' 'int main(void){case 1:return 0;}' 0 $true
    Check 'switch-default-outside' 'int main(void){default:return 0;}' 0 $true
    Check 'switch-continue-outside-loop' 'int main(void){switch(0){default:continue;}return 0;}' 0 $true
    Check 'switch-label-declaration' 'int main(void){switch(0){case 0:int n=1;}return 0;}' 0 $true
    Check 'switch-label-end' 'int main(void){switch(0){case 0:}return 0;}' 0 $true
    $switchCases = @(0..1022 | ForEach-Object { "case ${_}:return $_;" }) -join ''
    Check 'switch-1023-cases' "int main(void){switch(1000){$switchCases}return -1;}" 1000
    Check 'ce-precedence' 'int x=2+3*4;int main(void){return x;}' 14
    Check 'ce-parentheses' 'int x=(2+3)*4;int main(void){return x;}' 20
    Check 'ce-negative-division' 'int x=-23/5;int main(void){return x;}' -4
    Check 'ce-negative-remainder' 'int x=-23%5;int main(void){return x;}' -3
    Check 'ce-bitwise' 'int x=(7^3)|8;int main(void){return x;}' 12
    Check 'ce-complement' 'unsigned x=~0U;int main(void){return x==4294967295U;}' 1
    Check 'ce-wide' 'unsigned long long x=(1ULL<<63)+7;int main(void){return (x>>63)+(x&7);}' 8
    Check 'ce-wide-divide' 'unsigned long long x=18446744073709551615ULL/3;int main(void){return x==6148914691236517205ULL;}' 1
    Check 'ce-wide-signed' 'long long x=-4294967297LL/3;int main(void){return x==-1431655765LL;}' 1
    Check 'ce-unsigned-wrap' 'unsigned x=4294967295U+2;int main(void){return x;}' 1
    Check 'ce-unsigned-multiply' 'unsigned x=4294967295U*3U;int main(void){return x==4294967293U;}' 1
    Check 'ce-usual-conversion' 'long long x=-1LL+4294967295U;int main(void){return x==4294967294LL;}' 1
    Check 'ce-cast' 'int x=(signed char)255+(unsigned short)65537;int main(void){return x;}' 0
    Check 'ce-comparison' 'int x=(-1<1)+2*(-1<1U)+4*(4>=4)+8*(3!=4);int main(void){return x;}' 13
    Check 'ce-signed-shift' 'int x=-16>>2;int main(void){return x;}' -4
    Check 'ce-conditional' 'int x=0?7:2?8:9;int main(void){return x;}' 8
    Check 'ce-conditional-conversion' 'long long x=1?-1:1U;int main(void){return x==4294967295LL;}' 1
    Check 'ce-short-circuit-and' 'int x=0&&(1/0);int main(void){return x;}' 0
    Check 'ce-short-circuit-or' 'int x=1||(1/0);int main(void){return x;}' 1
    Check 'ce-conditional-unevaluated' 'int x=1?4:(2147483647+1);int main(void){return x;}' 4
    Check 'ce-sizeof-type' 'int x=sizeof(int[3])+sizeof(long long);int main(void){return x;}' 20
    Check 'ce-sizeof-object' 'int a[4];int x=sizeof a/sizeof a[0];int main(void){return x;}' 4
    Check 'ce-sizeof-unevaluated' 'int a;int x=sizeof(a++);int main(void){return x+a;}' 4
    Check 'ce-array-initializers' 'int a[3]={2+3,4*2,1<<4};int main(void){return a[0]+a[1]+a[2];}' 29
    Check 'ce-null-initializer' 'int *p=1-1;int main(void){return p==0;}' 1
    Check 'ce-divide-zero' 'int x=1/0;int main(void){return 0;}' 0 $true
    Check 'ce-signed-add-overflow' 'int x=2147483647+1;int main(void){return 0;}' 0 $true
    Check 'ce-signed-sub-overflow' 'int x=(-2147483647-1)-1;int main(void){return 0;}' 0 $true
    Check 'ce-signed-mul-overflow' 'int x=65536*65536;int main(void){return 0;}' 0 $true
    Check 'ce-wide-add-overflow' 'long long x=9223372036854775807LL+1;int main(void){return 0;}' 0 $true
    Check 'ce-wide-mul-overflow' 'long long x=4611686018427387904LL*2;int main(void){return 0;}' 0 $true
    Check 'ce-negate-overflow' 'int x=-(-2147483647-1);int main(void){return 0;}' 0 $true
    Check 'ce-divide-overflow' 'int x=(-2147483647-1)/-1;int main(void){return 0;}' 0 $true
    Check 'ce-shift-negative' 'int x=1<<-1;int main(void){return 0;}' 0 $true
    Check 'ce-shift-width' 'int x=1<<32;int main(void){return 0;}' 0 $true
    Check 'ce-shift-overflow' 'int x=1<<31;int main(void){return 0;}' 0 $true
    foreach ($parameterCount in @(5, 6, 7, 8, 15, 16, 17, 31, 32, 63, 64, 65, 126, 127)) {
        $parameterNames = @(1..$parameterCount | ForEach-Object { "int a$_" }) -join ','
        $argumentValues = @(1..$parameterCount) -join ','
        $weightedSum = @(1..$parameterCount | ForEach-Object { "a$_*$_" }) -join '+'
        $expectedSum = $parameterCount * ($parameterCount + 1) * (2 * $parameterCount + 1) / 6
        Check "abi-$parameterCount-parameters" "int f($parameterNames){int result=$weightedSum;return result;}int main(void){return f($argumentValues);}" $expectedSum
    }
    Check 'abi-stack-wide' 'long long f(int a,int b,int c,int d,long long e,unsigned long long g){return e+g;}int main(void){return f(1,2,3,4,4294967296LL,8589934592ULL)==12884901888LL;}' 1
    Check 'abi-stack-narrow' 'int f(int a,int b,int c,int d,signed char e,unsigned short g){return e+g;}int main(void){return f(1,2,3,4,255,65537);}' 0
    Check 'abi-stack-pointer' 'int f(int a,int b,int c,int d,int *p){*p+=a+b+c+d;return *p;}int main(void){int x=5;int n=f(1,2,3,4,&x);return n+x;}' 30
    Check 'abi-stack-array-parameter' 'int f(int a,int b,int c,int d,int p[][2]){return p[1][1];}int main(void){int x[2][2]={{1,2},{3,4}};return f(1,2,3,4,x);}' 4
    Check 'abi-stack-parameter-address' 'int f(int a,int b,int c,int d,int e){int *p=&e;*p=9;return e;}int main(void){return f(1,2,3,4,5);}' 9
    Check 'abi-stack-parameter-shadow' 'int f(int a,int b,int c,int d,int e){int n=e;{int e=8;n+=e;}return n+e;}int main(void){return f(1,2,3,4,5);}' 18
    Check 'abi-stack-nested' 'int f(int a,int b,int c,int d,int e){return a+b+c+d+e;}int main(void){return 3+f(1,f(1,2,3,4,5),3,f(2,3,4,5,6),5);}' 47
    Check 'abi-stack-recursion' 'int f(int n,int a,int b,int c,int d){if(n==0)return a+b+c+d;return f(n-1,a+1,b+2,c+3,d+4);}int main(void){return f(6,0,0,0,0);}' 60
    Check 'abi-stack-call-after-parameters' 'int g(int a,int b,int c,int d,int e){return a+b+c+d+e;}int f(int a,int b,int c,int d,int e){return g(9,8,7,6,5)+a+b+c+d+e;}int main(void){return f(1,2,3,4,5);}' 50
    Check 'abi-stack-call-count' 'int calls;int g(int n){calls++;return n;}int f(int a,int b,int c,int d,int e){return a+b+c+d+e;}int main(void){int n=f(g(1),g(2),g(3),g(4),g(5));return n+calls;}' 20
    Check 'abi-stack-alignment' 'int f(int a,int b,int c,int d,int e){return ((unsigned long long)&a&15)==8;}int main(void){return 3+f(1,2,3,4,5)+f(1,2,3,4,5);}' 5
    Check 'abi-stack-loop-cleanup' 'int f(int a,int b,int c,int d,int e){return e;}int main(void){int n=0;for(int i=0;i<10000;i++)n+=f(1,2,3,4,1);return n;}' 10000
    Check 'abi-stack-unnamed-prototype' 'int f(int,int,int,int,long long);int main(void){return f(1,2,3,4,4294967296LL);}int f(int a,int b,int c,int d,long long e){return e>>32;}' 1
    Check 'abi-stack-prototype-mismatch' 'int f(int,int,int,int,int);int f(int a,int b,int c,int d,long long e){return 0;}int main(void){return 0;}' 0 $true
    Check 'abi-stack-too-few' 'int f(int a,int b,int c,int d,int e){return e;}int main(void){return f(1,2,3,4);}' 0 $true
    Check 'abi-stack-too-many' 'int f(int a,int b,int c,int d,int e){return e;}int main(void){return f(1,2,3,4,5,6);}' 0 $true
    $parameterNames = @(1..128 | ForEach-Object { "int a$_" }) -join ','
    Check 'abi-parameter-limit' "int f($parameterNames){return 0;}int main(void){return 0;}" 0 $true
    $argumentValues = @(1..128) -join ','
    Check 'abi-argument-limit' "int main(void){return f($argumentValues);}" 0 $true
    Check 'phase-trigraph-braces' 'int main(void)??<return 7;??>' 7
    Check 'phase-trigraph-brackets' 'int main(void){int a??(2??)={3,8};return a??(1??);}' 8
    Check 'phase-trigraph-or' 'int main(void){return 2??!4;}' 6
    Check 'phase-trigraph-xor' "int main(void){return 7??'2;}" 5
    Check 'phase-trigraph-complement' 'int main(void){return ??-0;}' -1
    Check 'phase-trigraph-string' 'int main(void){return "??="[0];}' 35
    Check 'phase-trigraph-backslash' 'int main(void){return "??/n"[0];}' 10
    Check 'phase-trigraph-overlap' 'int main(void){return sizeof("???=");}' 3
    Check 'phase-trigraph-unrecognized' 'int main(void){return sizeof("??x");}' 4
    Check 'phase-trigraph-no-rescan' 'int main(void){return sizeof("??/??/n");}' 3
    Check 'phase-splice-keyword' ('in\@LF@t main(void){ret\@LF@urn 9;}'.Replace('@LF@', "`n")) 9
    Check 'phase-splice-identifier' ('int main(void){int va\@LF@lue=4;return val\@LF@ue;}'.Replace('@LF@', "`n")) 4
    Check 'phase-splice-number' ('int main(void){return 12\@LF@3;}'.Replace('@LF@', "`n")) 123
    Check 'phase-splice-operator' ('int main(void){int n=2;n+\@LF@+;return n;}'.Replace('@LF@', "`n")) 3
    Check 'phase-splice-string' ('int main(void){return sizeof("ab\@LF@cd");}'.Replace('@LF@', "`n")) 5
    Check 'phase-splice-character' ("int main(void){return 'a\@LF@';}".Replace('@LF@', "`n")) 97
    Check 'phase-splice-escape' ('int main(void){return "\x\@LF@41"[0];}'.Replace('@LF@', "`n")) 65
    Check 'phase-splice-line-comment' ('int main(void){// skip\@LF@return 1;@LF@return 8;}'.Replace('@LF@', "`n")) 8
    Check 'phase-splice-comment-start' ('int main(void){/\@LF@* comment */return 6;}'.Replace('@LF@', "`n")) 6
    Check 'phase-splice-comment-end' ('int main(void){/* comment *\@LF@/return 6;}'.Replace('@LF@', "`n")) 6
    Check 'phase-splice-line-comment-start' ('int main(void){/\@LF@/ comment@LF@return 6;}'.Replace('@LF@', "`n")) 6
    Check 'phase-trigraph-splice' ('int main(void){ret??/@LF@urn 5;}'.Replace('@LF@', "`n")) 5
    Check 'phase-splice-before-trigraph-rescan' ('int main(void){return sizeof("?\@LF@?=");}'.Replace('@LF@', "`n")) 4
    Check 'phase-splice-consecutive-lines' ('int main(void){ret\@LF@\@LF@urn 4;}'.Replace('@LF@', "`n")) 4
    Check 'phase-splice-only-final-backslash' ('int main(void){return sizeof("\\@LF@n");}'.Replace('@LF@', "`n")) 2
    Check 'phase-splice-no-recursive-splice' ('int main(void){return sizeof("\\@LF@@LF@n");}'.Replace('@LF@', "`n")) 0 $true
    Check 'phase-newline-crlf' ('int main(void){ret\@LF@urn 3;}'.Replace('@LF@', "`r`n")) 3
    Check 'phase-newline-cr' ('int main(void){ret\@LF@urn 3;}'.Replace('@LF@', "`r")) 3
    Check 'phase-trigraph-splice-crlf' ('int main(void){ret??/@LF@urn 3;}'.Replace('@LF@', "`r`n")) 3
    Check 'phase-digraph-braces' 'int main(void)<%return 4;%>' 4
    Check 'phase-digraph-array' 'int main(void)<%int a<:2:>=<%4,7%>;return a<:1:>;%>' 7
    Check 'phase-digraph-string-unchanged' 'int main(void){return sizeof("<: :> <% %> %: %:%:");}' 20
    Check 'phase-digraph-sizeof' 'int main(void){return sizeof(int<:3:>);}' 12
    Check 'phase-digraph-label' 'int main(void)<%goto done;done:return 6;%>' 6
    Check 'phase-splice-digraph' ('int main(void)<\@LF@%return 8;%\@LF@>'.Replace('@LF@', "`n")) 8
    Check 'phase-comment-separates-identifiers' 'int main(void){int a/**/b=1;return 0;}' 0 $true
    Check 'phase-comment-separates-operators' 'int main(void){int n=1;n+/**/+;return n;}' 0 $true
    Check 'phase-splice-space-not-allowed' ('int main(void){ret\ @LF@urn 2;}'.Replace('@LF@', "`n")) 0 $true
    Check 'phase-control-not-whitespace' ('int main(void){return' + [char]1 + '4;}') 0 $true
    Check 'phase-nul-not-eof' ('int main(void){return 0;}' + [char]0 + 'garbage') 0 $true
    Check 'goto-forward' 'int main(void){goto done;return 1;done:return 7;}' 7
    Check 'goto-backward' 'int main(void){int n=0;again:n++;if(n<5)goto again;return n;}' 5
    Check 'goto-many-forward' 'int f(int n){if(n==1)goto done;if(n==2)goto done;return 0;done:return n;}int main(void){return f(1)+f(2);}' 3
    Check 'goto-function-scope' 'int f(void){goto done;done:return 3;}int main(void){goto done;done:return f()+4;}' 7
    Check 'goto-object-namespace' 'int main(void){int done=9;goto done;done:return done;}' 9
    Check 'goto-function-namespace' 'int done(void){return 8;}int main(void){goto done;done:return done();}' 8
    Check 'goto-main-label' 'int main(void){goto main;main:return 6;}' 6
    Check 'goto-nested-exit' 'int main(void){int n=0;while(1){{int a[100];a[0]=4;n=a[0];goto done;}}done:return n;}' 4
    Check 'goto-enter-block' 'int main(void){goto inside;{int a;inside:a=12;return a;}}' 12
    Check 'goto-skip-initializer' 'int n;int f(void){n++;return 1;}int main(void){goto inside;{int a=f();inside:a=5;return a+n;}}' 5
    Check 'goto-label-block' 'int main(void){goto done;done:{return 8;}}' 8
    Check 'goto-label-null' 'int main(void){goto done;done:;return 9;}' 9
    Check 'goto-label-chain' 'int main(void){goto second;first:second:third:return 11;}' 11
    Check 'goto-unreferenced-label' 'int main(void){unused:return 4;}' 4
    Check 'goto-if-else' 'int main(void){int n=0;if(n)other:n=1;else done:n=3;return n;}' 3
    Check 'goto-for-continue' 'int main(void){int n=0;for(int i=0;i<5;i++){goto add;add:n++;continue;}return n;}' 5
    Check 'goto-do-condition' 'int main(void){int n=0;do{goto add;add:n++;}while(n<4);return n;}' 4
    Check 'goto-lookahead-comments' 'int main(void){int n=0;n /* lookahead */ =4;goto done;done /* colon */ :return n;}' 4
    Check 'goto-missing-label' 'int main(void){goto absent;return 0;}' 0 $true
    Check 'goto-other-function-label' 'int f(void){done:return 0;}int main(void){goto done;return 0;}' 0 $true
    Check 'goto-duplicate-label' 'int main(void){done:;done:return 0;}' 0 $true
    Check 'goto-duplicate-nested-label' 'int main(void){done:;{done:;}return 0;}' 0 $true
    Check 'goto-keyword-label' 'int main(void){goto while;return 0;}' 0 $true
    Check 'goto-no-identifier' 'int main(void){goto ;return 0;}' 0 $true
    Check 'goto-no-semicolon' 'int main(void){goto done done:return 0;}' 0 $true
    Check 'goto-label-declaration' 'int main(void){done:int n=1;return n;}' 0 $true
    Check 'goto-label-end-block' 'int main(void){return 0;done:}' 0 $true
    Check 'string-index' 'int main(void){return "abc"[1];}' 98
    Check 'string-pointer' 'int main(void){char *p="abc";return p[2];}' 99
    Check 'string-sizeof' 'int main(void){return sizeof "abc";}' 4
    Check 'string-empty' 'int main(void){return sizeof ""+""[0];}' 1
    Check 'string-concatenation' 'int main(void){return sizeof("ab" "cd")+"ab" "cd"[2];}' 104
    Check 'string-comment-concatenation' 'int main(void){return "a"/*join*/"b"[1];}' 98
    Check 'string-escapes' 'int main(void){return "\a\b\f\n\r\t\v"[0]+"\a\b\f\n\r\t\v"[6];}' 18
    Check 'string-octal' 'int main(void){return "\101\102"[1];}' 66
    Check 'string-hex' 'int main(void){return "\x41" "B"[1];}' 66
    Check 'string-octal-limit' 'int main(void){return sizeof "\1234"+"\1234"[1];}' 55
    Check 'string-embedded-null' 'int main(void){return sizeof "a\0b"+"a\0b"[2];}' 102
    Check 'string-address-array' 'int main(void){char (*p)[4]=&"abc";return (*p)[1]+sizeof *p;}' 102
    Check 'string-local-fixed' 'int main(void){char a[5]="abc";return a[2]+a[3]+a[4];}' 99
    Check 'string-local-exact' 'int main(void){char a[3]="abc";return sizeof a+a[2];}' 102
    Check 'string-local-inferred' 'int main(void){char a[]="abc";return sizeof a+a[1];}' 102
    Check 'string-local-inferred-brace' 'int main(void){char a[]={"ab",};return sizeof a+a[1];}' 101
    Check 'string-local-matrix' 'int main(void){char a[2][4]={"ab","cd"};return a[0][1]+a[1][0]+a[0][3]+a[1][3];}' 197
    Check 'string-local-pointer-array' 'int main(void){char *p[2]={"ab","cd"};return p[0][0]+p[1][1];}' 197
    Check 'string-global-pointer' 'char *p="abc";int main(void){return p[1];}' 98
    Check 'string-global-fixed' 'char a[5]="abc";int main(void){return a[2]+a[3]+a[4];}' 99
    Check 'string-global-inferred' 'char a[]="ab" "cd";int main(void){return sizeof a+a[3];}' 105
    Check 'string-global-inferred-brace' 'char a[]={"abc",};int main(void){return sizeof a;}' 4
    Check 'string-global-pointer-array' 'char *a[2]={"ab","cd"};int main(void){return a[1][1];}' 100
    Check 'string-return-static' 'char *f(void){return "ab";}int main(void){return f()==f() && f()[1]==98;}' 1
    Check 'string-putchar-loop' 'int main(void){char *p="hello";while(*p)__c0_putchar(*p++);return 0;}' 0 $false '' 'hello'
    Check 'string-copy-independent' 'int main(void){char a[]="abc";a[0]=88;return a[0]+"abc"[0];}' 185
    Check 'string-sizeof-no-side-effects' 'int main(void){int i=0;return sizeof("ab"[i++])+i;}' 1
    Check 'string-too-long' 'int main(void){char a[2]="abc";return 0;}' 0 $true
    Check 'string-wrong-array-type' 'int main(void){short a[3]="ab";return 0;}' 0 $true
    Check 'string-global-incompatible-pointer' 'int *p="abc";int main(void){return 0;}' 0 $true
    Check 'string-unterminated' 'int main(void){return "abc;}' 0 $true
    Check 'string-invalid-escape' 'int main(void){return "\q"[0];}' 0 $true
    Check 'string-empty-hex-escape' 'int main(void){return "\x"[0];}' 0 $true
    Check 'string-overflow-hex-escape' 'int main(void){return "\x100"[0];}' 0 $true
    Check 'string-overflow-octal-escape' 'int main(void){return "\777"[0];}' 0 $true
    Check 'init-local-scalar-brace' 'int main(void){int a={7};return a;}' 7
    Check 'init-local-scalar-nested' 'int main(void){int a={{7,},};return a;}' 7
    Check 'init-local-array' 'int main(void){int a[3]={2,5,7};return a[0]+a[1]+a[2];}' 14
    Check 'init-local-array-zero' 'int main(void){int a[5]={3};return a[0]+a[1]+a[4];}' 3
    Check 'init-local-array-trailing' 'int main(void){int a[2]={3,4,};return a[0]+a[1];}' 7
    Check 'init-local-expression' 'int f(int a){return a+2;}int main(void){int i=3;int a[2]={f(i),i+1};return a[0]*10+a[1];}' 54
    Check 'init-local-sequence' 'int main(void){int i=0;int a[3]={i++,i++,i++};return a[0]+a[1]*10+a[2]*100+i;}' 213
    Check 'init-local-prior-element' 'int main(void){int a[2]={5,a[0]+1};return a[1];}' 6
    Check 'init-local-char' 'int main(void){unsigned char a[3]={257,-1};return a[0]+a[1]+a[2];}' 256
    Check 'init-local-wide' 'int main(void){long long a[3]={4294967296LL,-7};return (a[0]>>32)+a[1]+a[2];}' -6
    Check 'init-local-pointer-array' 'int main(void){int a=3,b=7;int *p[3]={&a,&b};return *p[0]+*p[1]+(p[2]==0);}' 11
    Check 'init-local-matrix' 'int main(void){int a[2][3]={{1,2},{3}};return a[0][0]+a[0][1]+a[0][2]+a[1][0]+a[1][2];}' 6
    Check 'init-local-elision' 'int main(void){int a[2][3]={1,2,3,4};return a[0][2]*10+a[1][0]+a[1][1]+a[1][2];}' 34
    Check 'init-local-three-dimensions' 'int main(void){int a[2][2][2]={{{1},{2}},{{3,4},{5}}};return a[0][0][0]+a[0][1][0]+a[1][0][0]+a[1][0][1]+a[1][1][0]+a[1][1][1];}' 15
    Check 'init-local-reinitialize' 'int main(void){int s=0;for(int i=0;i<3;i++){int a[3]={1};a[2]+=i;s+=a[2];}return s;}' 3
    Check 'init-local-large-zero' 'int main(void){int a[8192]={7};return a[0]+a[4096]+a[8191];}' 7
    Check 'init-global-negative' 'int a=-7;int main(void){return a;}' -7
    Check 'init-global-char' 'unsigned char a=257;short b=-3;int main(void){return a+b;}' -2
    Check 'init-global-wide' 'unsigned long long a=18446744073709551615ULL;int main(void){return a==~0ULL;}' 1
    Check 'init-global-array' 'int a[4]={1,2,3};int main(void){return a[0]+a[1]+a[2]+a[3];}' 6
    Check 'init-global-matrix' 'int a[2][3]={{1},{2,3}};int main(void){return a[0][0]+a[0][1]+a[1][0]+a[1][1]+a[1][2];}' 6
    Check 'init-global-elision' 'int a[2][2]={1,2,3};int main(void){return a[0][1]*10+a[1][0]+a[1][1];}' 23
    Check 'init-global-pointer' 'int a=7;int *p=&a;int main(void){return *p;}' 7
    Check 'init-global-pointer-array' 'int a=3,b=7;int *p[3]={&a,&b};int main(void){return *p[0]+*p[1]+(p[2]==0);}' 11
    Check 'init-global-array-address' 'int a[3]={1,2,3};int (*p)[3]=&a;int main(void){return (*p)[2];}' 3
    Check 'init-global-array-decay' 'int a[3]={1,2,3};int *p=a;int main(void){return p[1];}' 2
    Check 'init-global-fragment-chain' 'int a=3;int f(void){return a;}int b=7;int main(void){return f()+b+c;}int c=9;' 0 $true
    Check 'init-global-after-function' 'int a=3;int f(void){return a;}int b=7;int main(void){return f()+b;}' 10
    Check 'init-global-once' 'int a=3;int f(void){return a++;}int main(void){int x=f();return x*10+f();}' 34
    Check 'init-global-scalar-brace' 'int a={7,};int main(void){return a;}' 7
    Check 'init-excess-array' 'int main(void){int a[2]={1,2,3};return 0;}' 0 $true
    Check 'init-excess-scalar' 'int main(void){int a={1,2};return 0;}' 0 $true
    Check 'init-empty-braces' 'int main(void){int a[2]={};return 0;}' 0 $true
    Check 'init-array-needs-braces' 'int main(void){int a[2]=1;return 0;}' 0 $true
    Check 'init-static-runtime-read' 'int a;int b=a;int main(void){return 0;}' 0 $true
    Check 'init-static-call' 'int f(void){return 3;}int a=f();int main(void){return 0;}' 0 $true
    Check 'init-static-unary-pointer' 'int a[2];int *p=+a;int main(void){return 0;}' 0 $true
    Check 'array-local' 'int main(void){int a[3];a[0]=2;a[1]=5;a[2]=7;return a[0]+a[1]+a[2];}' 14
    Check 'array-local-char' 'int main(void){unsigned char a[3];a[0]=1;a[1]=255;a[2]=3;a[1]++;return a[0]*100+a[1]*10+a[2];}' 103
    Check 'array-local-wide' 'int main(void){long long a[3];a[0]=1LL<<40;a[1]=7;return (a[0]>>40)+a[1];}' 8
    Check 'array-local-sizeof' 'int main(void){short a[7];return sizeof a;}' 14
    Check 'array-local-parameters' 'int f(int x,int y){int a[3];a[0]=x;a[1]=y;return a[0]+a[1];}int main(void){return f(4,7);}' 11
    Check 'array-local-scope' 'int main(void){int a=7,s=0;{int a[3];a[0]=2;s=a[0];}return a*10+s;}' 72
    Check 'array-local-reuse' 'int main(void){int s=0;{int a[4096];a[4095]=3;s+=a[4095];}{int b[4096];b[4095]=4;s+=b[4095];}return s;}' 7
    Check 'array-stack-probe' 'int main(void){int a[8192];a[0]=3;a[4096]=5;a[8191]=7;return a[0]+a[4096]+a[8191];}' 15
    Check 'array-stack-recursion' 'int f(int n){int a[2048];a[0]=n;a[2047]=n+1;if(n==0)return a[2047];return f(n-1)+a[0]+a[2047];}int main(void){return f(3);}' 16
    Check 'array-parenthesized-name' 'int main(void){int (a)=7;return a;}' 7
    Check 'array-parenthesized-pointer' 'int main(void){int a=7;int (*p)=&a;return *p;}' 7
    Check 'array-pointer-to-array' 'int main(void){int a[3];int (*p)[3]=&a;(*p)[1]=9;return a[1];}' 9
    Check 'array-address-sizeof' 'int main(void){int a[3];return sizeof *(&a);}' 12
    Check 'array-pointer-stride' 'int a[2][3];int main(void){int (*p)[3]=a;p++;(*p)[2]=7;return a[1][2];}' 7
    Check 'array-large-stride' 'int a[3][64];int main(void){int (*p)[64]=a;p++;p+=1;(*p)[63]=9;return a[2][63];}' 9
    Check 'array-pointer-difference' 'int a[4][5];int main(void){return (a+3)-(a+1);}' 2
    Check 'array-multidimensional-global' 'int a[2][3];int main(void){a[0][2]=4;a[1][0]=7;return a[0][2]+a[1][0];}' 11
    Check 'array-multidimensional-local' 'int main(void){int a[2][3];a[0][2]=4;a[1][0]=7;return a[0][2]+a[1][0];}' 11
    Check 'array-three-dimensions' 'int main(void){char a[2][3][4];a[1][2][3]=9;return a[1][2][3]+sizeof a;}' 33
    Check 'array-row-sizeof' 'int a[2][3];int main(void){return sizeof a+sizeof a[0]+sizeof &a;}' 44
    Check 'array-row-address' 'int a[2][3];int main(void){int (*p)[3]=&a[1];(*p)[2]=8;return a[1][2];}' 8
    Check 'array-of-pointers-local' 'int main(void){int a=3,b=7;int *p[2];p[0]=&a;p[1]=&b;return *p[0]+*p[1];}' 10
    Check 'array-of-array-pointers' 'int main(void){int a[3];int (*p[2])[3];p[0]=&a;(*p[0])[2]=9;return a[2];}' 9
    Check 'array-double-pointer-to-array' 'int main(void){int a[3];int (*p)[3]=&a;int (**q)[3]=&p;(**q)[1]=8;return a[1];}' 8
    Check 'array-type-interning' 'int main(void){int a[3];int (*p)[3]=&a;int (*q)[3]=p;return p==q;}' 1
    Check 'array-parameter-adjust' 'int f(int a[3]){return a[2];}int main(void){int a[3];a[2]=9;return f(a);}' 9
    Check 'array-parameter-incomplete' 'int f(int a[]){return a[1];}int main(void){int a[2];a[1]=7;return f(a);}' 7
    Check 'array-parameter-matrix' 'int f(int a[][3]){return a[1][2];}int main(void){int a[2][3];a[1][2]=8;return f(a);}' 8
    Check 'array-parameter-sizeof' 'int f(int a[3]){return sizeof a;}int main(void){int a[3];return f(a);}' 8
    Check 'array-unnamed-prototype' 'int f(int,short);int main(void){return f(3,4);}int f(int a,short b){return a+b;}' 7
    Check 'array-unnamed-pointer-prototype' 'int f(int (*)[3]);int main(void){int a[3];a[2]=9;return f(&a);}int f(int (*p)[3]){return (*p)[2];}' 9
    Check 'array-compatible-parameter-prototype' 'int f(int *p);int f(int p[3]){return p[1];}int main(void){int a[3];a[1]=5;return f(a);}' 5
    Check 'array-abstract-sizeof' 'int main(void){return sizeof(int[3])+sizeof(char[2][3])+sizeof(int(*)[3]);}' 26
    Check 'array-abstract-cast' 'int a[2][3];int main(void){void *v=a;int (*p)[3]=(int(*)[3])v;p[1][2]=7;return a[1][2];}' 7
    Check 'array-sizeof-no-call' 'int f(void);int main(void){return sizeof(f());}' 4
    Check 'array-assignment-rejected' 'int main(void){int a[3],b[3];a=b;return 0;}' 0 $true
    Check 'array-increment-rejected' 'int main(void){int a[3];a++;return 0;}' 0 $true
    Check 'array-shape-mismatch' 'int main(void){int a[3];int (*p)[4]=&a;return 0;}' 0 $true
    Check 'array-too-large-frame' 'int main(void){int a[20000];return 0;}' 0 $true
    Check 'array-size-overflow' 'int a[1000000][1000000];int main(void){return 0;}' 0 $true
    Check 'array-void-element' 'void a[3];int main(void){return 0;}' 0 $true
    Check 'array-incomplete-local' 'int main(void){int a[];return 0;}' 0 $true
    Check 'array-return-type' 'int f[3](void){return 0;}int main(void){return 0;}' 0 $true
    Check 'array-function-pointer-declaration' 'int (*p)(int);int main(void){return sizeof p;}' 8
    Check 'array-unnamed-definition' 'int f(int){return 0;}int main(void){return 0;}' 0 $true
    Check 'wide-four-register-parameters' 'long long f(long long a,long long b,long long c,long long d){return a+b+c+d;}int main(void){return f(1LL<<32,1LL<<33,1LL<<34,1LL<<35)==(15LL<<32);}' 1
    Check 'wide-pointer-register-high-bits' 'void *f(void *a,void *b,void *c,void *d){return d;}int main(void){return f(0,0,0,(void*)-1)==(void*)-1;}' 1
    Check 'wide-literal' 'int main(void){return 4294967296LL==((long long)1<<32);}' 1
    Check 'wide-unsigned-max' 'int main(void){return 18446744073709551615ULL==~0ULL;}' 1
    Check 'wide-hex-max' 'int main(void){return 0xffffffffffffffff==18446744073709551615ULL;}' 1
    Check 'wide-octal-max' 'int main(void){return 01777777777777777777777ULL==~0ULL;}' 1
    Check 'wide-unsuffixed-decimal' 'int main(void){return 9223372036854775807==9223372036854775807LL;}' 1
    Check 'wide-sign-extension' 'int main(void){int a=-1;long long b=a;return b==-1LL;}' 1
    Check 'wide-zero-extension' 'int main(void){unsigned a=4294967295U;long long b=a;return b==4294967295LL;}' 1
    Check 'wide-truncation' 'int main(void){long long a=4294967303LL;int b=a;return b;}' 7
    Check 'wide-char-truncation' 'int main(void){long long a=4294967551LL;return (signed char)a;}' -1
    Check 'wide-add' 'int main(void){return 4294967296LL+7==4294967303LL;}' 1
    Check 'wide-subtract' 'int main(void){return 4294967296LL-4294967297LL==-1LL;}' 1
    Check 'wide-multiply' 'int main(void){return 65536LL*65536LL==4294967296LL;}' 1
    Check 'wide-unsigned-wrap' 'int main(void){unsigned long long a=~0ULL;return a*3==a-2;}' 1
    Check 'wide-signed-divide' 'int main(void){return -4294967297LL/3==-1431655765LL;}' 1
    Check 'wide-signed-remainder' 'int main(void){return -4294967297LL%3;}' -2
    Check 'wide-unsigned-divide' 'int main(void){return ~0ULL/3==6148914691236517205ULL;}' 1
    Check 'wide-unsigned-remainder' 'int main(void){return ~0ULL%65536;}' 65535
    Check 'wide-shift-right' 'int main(void){return (1ULL<<63)>>63;}' 1
    Check 'wide-arithmetic-right' 'int main(void){return (-8LL>>2)==-2LL;}' 1
    Check 'wide-shift-left-type' 'int main(void){return (-8>>2ULL)==-2;}' 1
    Check 'wide-bitwise' 'int main(void){return ((0x100000001ULL|2)&~1ULL)^0x100000000ULL;}' 2
    Check 'wide-comparison' 'int main(void){return -1LL<4294967295U && ~0ULL>9223372036854775807LL;}' 1
    Check 'wide-truth' 'int main(void){long long a=4294967296LL;if(a)return !a+1;return 0;}' 1
    Check 'wide-conditional-true' 'int main(void){long long a=1?-1:1LL;return a==-1LL;}' 1
    Check 'wide-conditional-false' 'int main(void){long long a=0?1LL:-1;return a==-1LL;}' 1
    Check 'wide-conditional-unsigned' 'int main(void){return (1?-1:1ULL)==~0ULL;}' 1
    Check 'wide-compound' 'int main(void){unsigned long long a=1;a<<=40;a+=7;return a==1099511627783ULL;}' 1
    Check 'wide-postfix' 'int main(void){long long a=4294967296LL,b=0;b=a++;return b==4294967296LL && a==4294967297LL;}' 1
    Check 'wide-prefix' 'int main(void){long long a=4294967296LL;return --a==4294967295LL;}' 1
    Check 'wide-global' 'long long a;int main(void){a=4294967296LL;return a>>32;}' 1
    Check 'wide-array' 'long long a[3];int main(void){a[0]=7;a[1]=4294967296LL;a[2]=9;return (a[1]>>32)+a[0]+a[2];}' 17
    Check 'wide-pointer' 'int main(void){long long a=4294967296LL;long long *p=&a;*p+=7;return *p==4294967303LL;}' 1
    Check 'wide-parameter-return' 'long long f(long long a,unsigned b){return a+b;}int main(void){return f(4294967296LL,4294967295U)==8589934591LL;}' 1
    Check 'wide-pointer-cast' 'int main(void){void *p=(void*)4294967296ULL;return (unsigned long long)p==4294967296ULL;}' 1
    Check 'wide-pointer-difference-type' 'int a[4];int main(void){return sizeof((a+1)-a);}' 8
    Check 'wide-pointer-difference' 'int a[4];int main(void){return (a+1)-(a+3);}' -2
    Check 'wide-pointer-step' 'long long a[4];int main(void){long long *p=a;p+=2;return p-a;}' 2
    Check 'wide-sizeof-types' 'int main(void){return sizeof(char)+sizeof(short)+sizeof(int)+sizeof(long)+sizeof(long long)+sizeof(int*);}' 27
    Check 'wide-sizeof-array' 'short a[7];int main(void){return sizeof a;}' 14
    Check 'wide-sizeof-array-parentheses' 'char a[13];int main(void){return sizeof((a));}' 13
    Check 'wide-sizeof-decay' 'int a[7];int main(void){return sizeof(a+0)+sizeof((0,a));}' 16
    Check 'wide-sizeof-element' 'long long a[3];int main(void){return sizeof a[0];}' 8
    Check 'wide-sizeof-unevaluated' 'int main(void){int i=3;return sizeof(i++)+i;}' 7
    Check 'wide-sizeof-null-deref' 'int main(void){int *p=0;return sizeof *p;}' 4
    Check 'wide-sizeof-call' 'int g;long long f(void){g=1;return 0;}int main(void){return sizeof(f())+g;}' 8
    Check 'wide-sizeof-result-type' 'int main(void){return sizeof(sizeof(int));}' 8
    Check 'wide-invalid-suffix' 'int main(void){return 1lL;}' 0 $true
    Check 'wide-signed-literal-overflow' 'int main(void){return 9223372036854775808LL;}' 0 $true
    Check 'wide-void-sizeof' 'int main(void){return sizeof(void);}' 0 $true
    Check 'wide-global-high-initializer' 'long long a=4294967296LL;int main(void){return a>>32;}' 1
    Check 'wide-large-array-bound' 'int a[4294967297ULL];int main(void){return 0;}' 0 $true
    Check 'pointer-conditional-incompatible-null' 'int main(void){int *p=1?(int*)0:(char*)0;return 0;}' 0 $true
    Check 'pointer-void-relational' 'int main(void){int a;void *p=&a;return p<&a;}' 0 $true
    Check 'pointer-local' 'int main(void){int a=7;int *p=&a;return *p;}' 7
    Check 'pointer-local-write' 'int main(void){int a=7;int *p=&a;*p=12;return a;}' 12
    Check 'pointer-address-uninitialized' 'int main(void){int a;int *p=&a;*p=13;return a;}' 13
    Check 'pointer-global' 'int g;int *p;int main(void){p=&g;*p=9;return g;}' 9
    Check 'pointer-double' 'int main(void){int a=7;int *p=&a;int **q=&p;**q=13;return a;}' 13
    Check 'pointer-null' 'int main(void){int *p=0;return p==0;}' 1
    Check 'pointer-null-left' 'int main(void){int a=2;int *p=&a;return 0!=p;}' 1
    Check 'pointer-address-deref-null' 'int main(void){int *p=0;return &*p==p;}' 1
    Check 'pointer-truth' 'int main(void){int a=1;int *p=&a;if(p)return !p+1;return 0;}' 1
    Check 'pointer-char' 'int main(void){signed char a=-7;signed char *p=&a;return *p;}' -7
    Check 'pointer-short' 'int main(void){unsigned short a=65535;unsigned short *p=&a;*p+=2;return a;}' 1
    Check 'pointer-parameter' 'int f(int *p){*p+=3;return *p;}int main(void){int a=7;return f(&a)+2;}' 12
    Check 'pointer-return' 'int g;int *f(void){return &g;}int main(void){*f()=11;return g;}' 11
    Check 'pointer-prototype' 'int *f(int *p);int main(void){int a=9;return *f(&a);}int *f(int *p){return p;}' 9
    Check 'pointer-void-parameter' 'int f(void *v,int n){int *p=v;return *p+n;}int main(void){int a=7;return f(&a,3);}' 10
    Check 'pointer-void-roundtrip' 'int main(void){int a=5;void *v=&a;int *p=v;return *p;}' 5
    Check 'pointer-array-decay' 'int a[4];int main(void){int *p=a;a[2]=9;return p[2];}' 9
    Check 'pointer-index-commuted' 'int a[4];int main(void){a[2]=9;return 2[a];}' 9
    Check 'pointer-arithmetic' 'int a[4];int main(void){int *p=a+3;a[1]=7;return *(p-2);}' 7
    Check 'pointer-char-stride' 'unsigned char a[4];int main(void){unsigned char *p=a;a[1]=255;p++;return *p;}' 255
    Check 'pointer-postfix' 'int a[4];int main(void){int *p=a;int *q=p++;return q==a && p==a+1;}' 1
    Check 'pointer-prefix' 'int a[4];int main(void){int *p=a;int *q=++p;return q==a+1 && p==q;}' 1
    Check 'pointer-decrement' 'int a[4];int main(void){int *p=a+2;p--;--p;return p==a;}' 1
    Check 'pointer-compound' 'int a[4];int main(void){int *p=a;p+=3;p-=2;return p==a+1;}' 1
    Check 'pointer-array-of-pointers' 'int *a[2];int main(void){int x=7,y=9;a[0]=&x;a[1]=&y;return *a[0]+*a[1];}' 16
    Check 'pointer-nested-subscript' 'int a[2];int *b[2];int main(void){b[0]=a;b[0][1]=12;return a[1];}' 12
    Check 'pointer-element-address' 'int a[4];int main(void){int i=0;int *p=&a[i++];*p=9;return i*10+a[0];}' 19
    Check 'pointer-conditional' 'int main(void){int a=7,b=9;int *p=0?&a:&b;return *p;}' 9
    Check 'pointer-conditional-null' 'int main(void){int a=7;int *p=1?&a:0;return *p;}' 7
    Check 'pointer-relational' 'int a[4];int main(void){return a<a+1 && a+2>=a+1 && !(a+3<a);}' 1
    Check 'pointer-high-bits' 'int main(void){int *p=(int*)-1;int *q=(int*)4294967295U;return p!=q;}' 1
    Check 'pointer-mixed-declarators' 'int main(void){int a=7,*p=&a,**q=&p;return **q;}' 7
    Check 'pointer-global-mixed' 'int *p,g;int main(void){p=&g;*p=8;return g;}' 8
    Check 'pointer-shadow' 'int main(void){int a=7;int *p=&a;{int p=2;p++;}return *p;}' 7
    Check 'pointer-nonzero-implicit' 'int main(void){int *p=1;return 0;}' 0 $true
    Check 'pointer-incompatible' 'int main(void){char c;int *p=&c;return 0;}' 0 $true
    Check 'pointer-deref-integer' 'int main(void){return *1;}' 0 $true
    Check 'pointer-deref-void' 'int main(void){void *p=0;return *p;}' 0 $true
    Check 'pointer-address-rvalue' 'int main(void){int *p=&1;return 0;}' 0 $true
    Check 'pointer-add-two' 'int a[2];int main(void){return a+a;}' 0 $true
    Check 'pointer-void-increment' 'int main(void){void *p=0;p++;return 0;}' 0 $true
    Check 'pointer-integer-return' 'int main(void){int a;return &a;}' 0 $true
    Check 'type-char-truncate' 'int main(void){char c=257;return c;}' 1
    Check 'type-char-signed' 'int main(void){char c=255;return c;}' -1
    Check 'type-signed-char' 'int main(void){signed char c=128;return c;}' -128
    Check 'type-unsigned-char' 'int main(void){unsigned char c=-1;return c;}' 255
    Check 'type-short-signed' 'int main(void){short s=65535;return s;}' -1
    Check 'type-unsigned-short' 'int main(void){unsigned short s=-1;return s;}' 65535
    Check 'type-promotions' 'int main(void){unsigned short a=65535;unsigned char b=255;return (a+b)/256;}' 256
    Check 'type-long' 'int main(void){signed long int a=-17;return a/5;}' -3
    Check 'type-specifier-order' 'int main(void){int unsigned long a=4294967295UL;return a>0;}' 1
    Check 'type-uint-comparison' 'int main(void){unsigned a=4294967295U;return (a>1)+(a>=1)*2+(a<1)*4+(a<=1)*8;}' 3
    Check 'type-uint-division' 'int main(void){unsigned a=4294967295U;return a/65535;}' 65537
    Check 'type-uint-remainder' 'int main(void){unsigned a=4294967295U;return a%65536;}' 65535
    Check 'type-uint-shift' 'int main(void){unsigned a=2147483648U;return a>>31;}' 1
    Check 'type-shift-left-type' 'int main(void){int a=-8;return a>>1U;}' -4
    Check 'type-usual-conversion' 'int main(void){unsigned a=1;return -1<a;}' 0
    Check 'type-long-unsigned-conversion' 'int main(void){long a=-1;unsigned b=1;return a<b;}' 0
    Check 'type-hex-unsigned' 'int main(void){return 0xffffffff>0;}' 1
    Check 'type-decimal-ulong-c89' 'int main(void){return 4294967295>0;}' 1
    Check 'type-octal-unsigned' 'int main(void){return 037777777777>>31;}' 1
    Check 'type-suffix-case-order' 'int main(void){return 0x80000000lU>>31;}' 1
    Check 'type-char-cast' 'int main(void){return (signed char)255;}' -1
    Check 'type-unsigned-cast' 'int main(void){return ((unsigned)-1)>>31;}' 1
    Check 'type-cast-precedence' 'int main(void){return (unsigned char)256+1;}' 1
    Check 'type-char-assignment-value' 'int main(void){char c;int i=0;i=(c=257);return i*10+c;}' 11
    Check 'type-char-compound' 'int main(void){unsigned char c=250;return c+=10;}' 4
    Check 'type-short-compound' 'int main(void){unsigned short c=65530;return c+=10;}' 4
    Check 'type-char-postfix' 'int main(void){unsigned char c=255;int old=0;old=c++;return old*10+c;}' 2550
    Check 'type-char-prefix' 'int main(void){unsigned char c=255;return ++c;}' 0
    Check 'type-global-char' 'char c;int x;int main(void){x=42;c=-1;return x*10+c;}' 419
    Check 'type-global-short' 'short s;int x;int main(void){x=42;s=-2;return x*10+s;}' 418
    Check 'type-char-array-width' 'unsigned char a[3];int main(void){a[0]=1;a[1]=255;a[2]=3;a[1]++;return a[0]*100+a[1]*10+a[2];}' 103
    Check 'type-short-array-width' 'unsigned short a[3];int main(void){a[0]=1;a[1]=65535;a[2]=3;a[1]++;return a[0]*100+a[1]*10+a[2];}' 103
    Check 'type-array-signed-load' 'signed char a[2];int main(void){a[1]=255;return a[1];}' -1
    Check 'type-array-compound-once' 'unsigned char a[2];int main(void){int i=0;a[0]=250;a[i++]+=10;return a[0]*10+i;}' 41
    Check 'type-parameter-conversion' 'int f(unsigned char c,short s){return c+s;}int main(void){return f(257,65535);}' 0
    Check 'type-return-conversion' 'unsigned char f(void){return 257;}int main(void){return f();}' 1
    Check 'type-signed-return' 'signed char f(void){return 255;}int main(void){return f();}' -1
    Check 'type-return-uint' 'unsigned f(void){return -1;}int main(void){return f()>0;}' 1
    Check 'type-prototype' 'unsigned char f(unsigned short x);int main(void){return f(65535);}unsigned char f(unsigned short x){return x;}' 255
    Check 'type-conditional-common' 'int main(void){return (1?-1:1U)>0;}' 1
    Check 'type-conditional-promotion' 'int main(void){signed char a=-1;unsigned char b=2;return (1?a:b)<0;}' 1
    Check 'type-shadow' 'int main(void){int a=300;{unsigned char a=255;a++;}return a;}' 300
    Check 'type-for-char' 'int main(void){int s=0;for(unsigned char i=0;i<3;i++)s+=i;return s;}' 3
    Check 'type-void-function' 'int g;void f(void){g=7;return;}int main(void){f();return g;}' 7
    Check 'type-void-cast' 'int main(void){int i=0;(void)(i=3);return i;}' 3
    Check 'type-void-conditional' 'int g;void f(void){g=3;}void h(void){g=7;}int main(void){0?f():h();return g;}' 7
    Check 'type-invalid-combination' 'int main(void){unsigned signed a;return 0;}' 0 $true
    Check 'type-invalid-char-int' 'int main(void){char int a;return 0;}' 0 $true
    Check 'type-duplicate-specifier' 'int main(void){int int a;return 0;}' 0 $true
    Check 'type-void-object' 'int main(void){void a;return 0;}' 0 $true
    Check 'type-void-condition' 'void f(void){}int main(void){if(f())return 1;return 0;}' 0 $true
    Check 'type-void-value' 'void f(void){}int main(void){return f();}' 0 $true
    Check 'type-void-return-expression' 'void f(void){return 1;}int main(void){f();return 0;}' 0 $true
    Check 'type-missing-return-value' 'int f(void){return;}int main(void){return f();}' 0 $true
    Check 'type-conflicting-return' 'char f(void);int f(void){return 0;}int main(void){return 0;}' 0 $true
    Check 'type-conflicting-parameter' 'int f(char x);int f(int x){return x;}int main(void){return 0;}' 0 $true
    Check 'type-duplicate-suffix' 'int main(void){return 1UU;}' 0 $true
    Check 'type-cast-not-lvalue' 'int main(void){int a=0;(int)a=2;return 0;}' 0 $true
    Check 'scope-for-declaration' 'int main(void){int s=0;for(int i=0;i<4;i++)s+=i;return s;}' 6
    Check 'scope-for-shadow' 'int main(void){int i=8,s=0;for(int i=0;i<3;i++)s+=i;return i*10+s;}' 83
    Check 'scope-for-multiple' 'int main(void){int s=0;for(int i=0,j=3;i<3;i++,j--)s+=i+j;return s;}' 9
    Check 'scope-for-body-shadow' 'int main(void){int s=0;for(int i=0;i<3;i++){int i=7;s+=i;}return s;}' 21
    Check 'scope-for-continue' 'int main(void){int s=0;for(int i=0;i<4;i++){if(i==1)continue;s+=i;}return s;}' 5
    Check 'scope-for-outside-use' 'int main(void){for(int i=0;i<3;i++);return i;}' 0 $true
    Check 'scope-shadow-local' 'int main(void){int a=3,s=0;{int a=7;s=a;}return s*10+a;}' 73
    Check 'scope-shadow-global' 'int a;int main(void){a=4;{int a=8;a++;}return a;}' 4
    Check 'scope-shadow-array' 'int a[2];int main(void){int s=0;a[1]=4;{int a=7;s=a;}return s*10+a[1];}' 74
    Check 'scope-shadow-function' 'int f(void){return 3;}int main(void){int s=0;{int f=7;s=f;}return s*10+f();}' 73
    Check 'scope-shadow-parameter' 'int f(int a){int s=0;{int a=7;s=a;}return s*10+a;}int main(void){return f(3);}' 73
    Check 'scope-parameter-global' 'int a;int f(int a){return a;}int main(void){a=8;return f(3)*10+a;}' 38
    Check 'scope-siblings' 'int main(void){int s=0;{int a=2;s+=a;}{int a=3;s+=a;}return s;}' 5
    Check 'scope-three-levels' 'int main(void){int a=1,s=0;{int a=2;{int a=3;s+=a;}s+=a;}return s*10+a;}' 51
    Check 'scope-loop-initialization' 'int main(void){int i=0,s=0;while(i<3){int a=5;s+=a;i++;}return s;}' 15
    Check 'scope-break-restore' 'int main(void){int a=4;while(1){int a=8;break;}return a;}' 4
    Check 'scope-continue-restore' 'int main(void){int a=4,i=0;while(i++<3){int a=8;continue;}return a;}' 4
    Check 'scope-mixed-declaration' 'int main(void){int a=2;a++;int b=4;return a+b;}' 7
    Check 'scope-duplicate-nested' 'int main(void){{int a;int a;}return 0;}' 0 $true
    Check 'scope-duplicate-parameter' 'int f(int a){int a;return 0;}int main(void){return f(1);}' 0 $true
    Check 'scope-outside-use' 'int main(void){{int a=1;}return a;}' 0 $true
    Check 'scope-declaration-not-statement' 'int main(void){if(1)int a;return 0;}' 0 $true
    Check 'scope-shadow-function-call' 'int f(void){return 3;}int main(void){int f=7;return f();}' 0 $true
    Check 'scope-reuse-slots' ('int main(void){int s=0;' + ('{int a=1;s+=a;}' * 80) + 'return s;}') 80
    Check 'do-once' 'int main(void){int i=0;do i++;while(0);return i;}' 1
    Check 'do-repeat' 'int main(void){int i=0;do{i++;}while(i<4);return i;}' 4
    Check 'do-empty' 'int main(void){int i=0;do;while(++i<3);return i;}' 3
    Check 'do-continue-condition' 'int main(void){int i=0,s=0;do{i++;if(i<3)continue;s+=i;}while(i<3);return s;}' 3
    Check 'do-continue-false' 'int main(void){int i=0;do{i++;continue;}while(0);return i;}' 1
    Check 'do-continue-side-effect' 'int main(void){int i=0;do{continue;}while(++i<4);return i;}' 4
    Check 'do-break-skips-condition' 'int main(void){int i=0;do{break;}while(++i);return i;}' 0
    Check 'do-break-chain' 'int main(void){int i=0;do{if(i==1)break;i++;if(i==2)break;}while(1);return i;}' 1
    Check 'do-nested-break' 'int main(void){int i=0,j=0;do{i++;do{j++;break;}while(1);}while(i<3);return i*10+j;}' 33
    Check 'do-nested-continue' 'int main(void){int i=0,j=0;do{i++;do{j++;continue;}while(j<2);continue;}while(i<3);return i*10+j;}' 34
    Check 'do-in-for' 'int main(void){int i=0,s=0;for(i=0;i<3;i++){do{s++;continue;}while(0);continue;}return s;}' 3
    Check 'while-in-do' 'int main(void){int i=0,j=0;do{i++;while(j<2){j++;continue;}continue;}while(i<3);return i*10+j;}' 32
    Check 'do-condition-comma' 'int main(void){int i=0,s=0;do{s+=i;}while(i++,i<4);return s;}' 6
    Check 'do-return' 'int main(void){do{return 7;}while(1);return 0;}' 7
    Check 'do-if-else' 'int main(void){int i=0;if(1)do i++;while(0);else i=9;return i;}' 1
    Check 'do-missing-while' 'int main(void){do{}return 0;}' 0 $true
    Check 'do-missing-semicolon' 'int main(void){do{}while(0)return 0;}' 0 $true
    Check 'do-empty-condition' 'int main(void){do{}while();return 0;}' 0 $true
    Check 'do-missing-body' 'int main(void){do while(0);return 0;}' 0 $true
    Check 'do-restores-loop-depth' 'int main(void){do{}while(0);continue;return 0;}' 0 $true
    Check 'comma-result' 'int main(void){return 1,2,3;}' 3
    Check 'comma-sequence' 'int main(void){int a=0;return a=3,a+=4,a*2;}' 14
    Check 'comma-assignment-precedence' 'int main(void){int a=0,b=0;a=1,b=2;return a*10+b;}' 12
    Check 'comma-parentheses' 'int main(void){int a=0,b=0;b=(a=3,a+4);return a*10+b;}' 37
    Check 'comma-argument' 'int f(int a,int b){return a*10+b;}int main(void){return f((1,2),3);}' 23
    Check 'comma-argument-separators' 'int f(int a,int b){return a*10+b;}int main(void){return f(2,3);}' 23
    Check 'comma-initializer-separators' 'int main(void){int a=2,b=3;return a*10+b;}' 23
    Check 'comma-initializer-parentheses' 'int main(void){int a=(1,2),b=3;return a*10+b;}' 23
    Check 'comma-index' 'int a[4];int main(void){int i=0;a[(i=2,i)] = 7;return a[2]*10+i;}' 72
    Check 'comma-conditional-middle' 'int main(void){int a=0,b=0;b=1?a=3,a+4:9;return a*10+b;}' 37
    Check 'comma-conditional-outside' 'int main(void){int a=0;return 0?1:2,a=7,a;}' 7
    Check 'comma-short-circuit' 'int main(void){int a=0;0&&(a=1,a=2);return a;}' 0
    Check 'comma-if' 'int main(void){int a=0;if(a=3,a==3)return a;return 9;}' 3
    Check 'comma-while' 'int main(void){int i=0,s=0;while(i++,i<4)s+=i;return s;}' 6
    Check 'comma-for' 'int main(void){int i=0,j=0,s=0;for(i=0,j=5;i<3,j>2;i++,j--)s+=i+j;return s;}' 15
    Check 'comma-for-continue' 'int main(void){int i=0,j=0;for(i=0,j=0;i<3;i++,j+=2)continue;return j;}' 6
    Check 'comma-postfix-sequence' 'int main(void){int a=1;return a++,a++,a;}' 3
    Check 'comma-invalid-lvalue' 'int main(void){int a=0,b=0;(a,b)=2;return 0;}' 0 $true
    Check 'comma-invalid-increment' 'int main(void){int a=0,b=0;return (a,b)++;}' 0 $true
    Check 'comma-missing-rhs' 'int main(void){return 1,;}' 0 $true
    Check 'comma-missing-lhs' 'int main(void){return ,1;}' 0 $true
    Check 'prefix-inc-value' 'int main(void){int a=5,b=0;b=++a;return a*10+b;}' 66
    Check 'prefix-dec-value' 'int main(void){int a=5,b=0;b=--a;return a*10+b;}' 44
    Check 'postfix-inc-value' 'int main(void){int a=5,b=0;b=a++;return a*10+b;}' 65
    Check 'postfix-dec-value' 'int main(void){int a=5,b=0;b=a--;return a*10+b;}' 45
    Check 'increment-global' 'int g;int main(void){int b=0;g=3;b=g++;return g*10+b;}' 43
    Check 'increment-global-prefix' 'int g;int main(void){g=3;return --g;}' 2
    Check 'increment-array-once' 'int a[4];int main(void){int i=0,b=0;a[0]=7;b=a[i++]++;return b*100+a[0]*10+i;}' 781
    Check 'increment-array-prefix' 'int a[4];int main(void){int i=0,b=0;a[1]=7;b=++a[++i];return b*100+a[1]*10+i;}' 881
    Check 'decrement-array-once' 'int a[4];int main(void){int i=2,b=0;a[2]=7;b=a[i--]--;return b*100+a[2]*10+i;}' 761
    Check 'increment-array-call' 'int a[4];int g;int f(void){g+=1;return 2;}int main(void){int b=0;a[2]=9;b=a[f()]++;return b*100+a[2]*10+g;}' 1001
    Check 'increment-parentheses' 'int main(void){int a=2,b=0;b=(a)++;return a*10+b;}' 32
    Check 'increment-unary-precedence' 'int main(void){int a=2,b=0;b=-a++;return a*10+b;}' 28
    Check 'increment-maximal-munch' 'int main(void){int a=2,b=3,c=0;c=a+++b;return a*10+c;}' 35
    Check 'increment-for' 'int main(void){int i=0,s=0;for(i=0;i<5;i++)s+=i;return s;}' 10
    Check 'increment-short-circuit' 'int main(void){int a=0;0&&a++;1||++a;return a;}' 0
    Check 'increment-conditional' 'int main(void){int a=2,b=3,c=0;c=0?a++:b++;return a*100+b*10+c;}' 243
    Check 'increment-negative' 'int main(void){int a=-3,b=0;b=a++;return a*10+b;}' -23
    Check 'increment-literal' 'int main(void){return 1++;}' 0 $true
    Check 'increment-prefix-literal' 'int main(void){return ++1;}' 0 $true
    Check 'increment-result-lvalue' 'int main(void){int a=0;a++=2;return a;}' 0 $true
    Check 'increment-prefix-result-lvalue' 'int main(void){int a=0;++a=2;return a;}' 0 $true
    Check 'increment-chain' 'int main(void){int a=0;return a++++;}' 0 $true
    Check 'increment-prefix-postfix' 'int main(void){int a=0;return ++a++;}' 0 $true
    Check 'increment-call-result' 'int f(void){return 2;}int main(void){return f()++;}' 0 $true
    Check 'compound-add' 'int main(void){int a=7;return a+=5;}' 12
    Check 'compound-sub' 'int main(void){int a=7;return a-=5;}' 2
    Check 'compound-mul' 'int main(void){int a=7;return a*=5;}' 35
    Check 'compound-div' 'int main(void){int a=-17;return a/=5;}' -3
    Check 'compound-mod' 'int main(void){int a=-17;return a%=5;}' -2
    Check 'compound-and' 'int main(void){int a=14;return a&=11;}' 10
    Check 'compound-or' 'int main(void){int a=8;return a|=3;}' 11
    Check 'compound-xor' 'int main(void){int a=14;return a^=11;}' 5
    Check 'compound-shl' 'int main(void){int a=3;return a<<=4;}' 48
    Check 'compound-shr' 'int main(void){int a=-32;return a>>=3;}' -4
    Check 'compound-chain' 'int main(void){int a=2,b=3;a+=b*=4;return a+b;}' 26
    Check 'compound-global' 'int g;int main(void){g=4;g*=3;return g;}' 12
    Check 'compound-array-once' 'int a[4];int main(void){int i=0;a[1]=5;a[i=i+1]+=7;return a[1]*10+i;}' 121
    Check 'compound-array-call' 'int a[4];int f(void){return 6;}int main(void){a[2]=7;a[2]*=f();return a[2];}' 42
    Check 'compound-array-nested' 'int a[4];int main(void){a[1]=3;a[2]=4;a[1]+=a[2]*=2;return a[1]*10+a[2];}' 118
    Check 'conditional-true' 'int main(void){return -1?17:3;}' 17
    Check 'conditional-false' 'int main(void){return 0?17:3;}' 3
    Check 'conditional-right-association' 'int main(void){return 1?0:1?2:3;}' 0
    Check 'conditional-nested-middle' 'int main(void){return 1?0?2:3:4;}' 3
    Check 'conditional-logical-precedence' 'int main(void){return 1||0?7:8;}' 7
    Check 'conditional-assignment-middle' 'int main(void){int a=0,b=0;b=1?a=12:3;return b+a;}' 24
    Check 'conditional-short-true' 'int main(void){int a=0;return (1?5:(a=9))+a;}' 5
    Check 'conditional-short-false' 'int main(void){int a=0;return (0?(a=9):5)+a;}' 5
    Check 'conditional-compound' 'int main(void){int a=2;a+=0?7:3;return a;}' 5
    Check 'compound-invalid-lvalue' 'int main(void){1+=3;return 0;}' 0 $true
    Check 'conditional-invalid-lvalue' 'int main(void){int a=0,b=0;(1?a:b)=3;return 0;}' 0 $true
    Check 'conditional-missing-colon' 'int main(void){return 1?2;}' 0 $true
    Check 'conditional-missing-middle' 'int main(void){return 1?:2;}' 0 $true
    Check 'conditional-missing-last' 'int main(void){return 1?2:;}' 0 $true
}
if ($CompilerKind -eq 'C1') {
    . (Join-Path $PSScriptRoot '../checks/tests.ps1')
    . (Join-Path $PSScriptRoot '../time-checks/tests-all.ps1')
    foreach ($feature in 'remove-directory', 'system-command', 'wide-literals', 'multichar') {
        . (Join-Path $PSScriptRoot ('../feature-checks/' + $feature + '.ps1'))
    }
}
if ($CompilerKind -eq 'C1') {
    foreach ($part in 'exp', 'log', 'hyperbolic', 'atan', 'asin', 'atan2', 'trig', 'pow', 'expm1', 'log1p') {
        . (Join-Path $PSScriptRoot ('../math-checks/' + $part + '/tests.ps1'))
    }
}
"$script:passed $CompilerKind tests passed."
