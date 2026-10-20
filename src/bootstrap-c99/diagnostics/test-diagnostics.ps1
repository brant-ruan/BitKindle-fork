param([Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$ObjectMode)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

$ErrorActionPreference = 'Stop'
$testRoot = if ($ObjectMode) {
    'diagnostic-object-tests'
}
else {
    'diagnostic-tests'
}
$cases = [ordered]@{
    hexlower = @('int main(void){return 0x1.5p5 != 42.0;}', 1, $false)
    hexupper = @('int main(void){return 0X1.5P5 != 42.0;}', 1, $false)
    hexfloat = @('int main(void){return 0x1.5p5f != 42.0f;}', 1, $false)
    hexlong = @('int main(void){return 0x1.5p5L != 42.0L;}', 1, $false)
    decimalfloat = @('int main(void){return 4.2e1 != 42.0;}', 0, $false)
    inactivefloat = @("#if 0`nfloat x=0x1.5p5;`n#endif`nint main(void){return 0;}", 0, $false)
    mixed = @('int main(void){int n=1;n++;int m=40;return n+m!=42;}', 1, $false)
    forloop = @('int main(void){int n=0;for(int i=0;i<3;i++)n+=i;return n!=3;}', 1, $false)
    nested = @('int main(void){int n=1;n++;{int m=40;return n+m!=42;}}', 0, $false)
    repeated = @('int main(void){;int n=20;int m=22;return n+m!=42;}', 2, $false)
    errorafter = @('int main(void){;int n=0;return missing;}', 1, $true)
    member = @('struct S{int n;};int main(void){struct S s={.n=42};return s.n!=42;}', 1, $false)
    array = @('int main(void){int a[3]={[2]=42};return a[0]||a[2]!=42;}', 1, $false)
    inferred = @('int main(void){int a[]={[2]=42};return sizeof(a)!=12||a[2]!=42;}', -1, $false)
    compound = @('int main(void){int *p=&(int){42};return *p!=42;}', 1, $false)
    staticcompound = @('int *p=&(int){42};int main(void){return *p!=42;}', 1, $false)
    sizeofcompound = @('int main(void){int n=0;return sizeof((int){n++})!=4||n;}', 1, $false)
    positional = @('struct S{int a[2];int n;};int main(void){struct S s={{20,21},1};return s.a[1]+s.n!=22;}', 0, $false)
    subscript = @('int a[2]={20,42};int main(void){int n=a[1];return n!=42;}', 0, $false)
    memberexpression = @('struct S{int n;};int main(void){struct S s={42};int n=(s).n;return n!=42;}', 0, $false)
    sizeoftype = @('int main(void){int n=sizeof(int);int a[2]={20,22};return n!=4||sizeof a!=8;}', 0, $false)
    enumcomma = @('enum E{A=20,B=22,};int main(void){return A+B!=42;}', 1, $false)
    enumplain = @('enum E{A=20,B=22};int main(void){return A+B!=42;}', 0, $false)
    flexible = @('struct S{int n;char data[];};int main(void){return sizeof(struct S)!=4;}', 1, $false)
    incompletearray = @('extern int a[];int a[2]={20,22};int main(void){return a[0]+a[1]!=42;}', 0, $false)
    arraystatic = @('int f(int a[static 1]){return a[0];}int main(void){int a[1]={42};return f(a)!=42;}', 1, $false)
    arrayconst = @('int f(int a[const 1]){return a[0];}int main(void){int a[1]={42};return f(a)!=42;}', 1, $false)
    arrayelementconst = @('int f(const int a[1]){return a[0];}int main(void){int a[1]={42};return f(a)!=42;}', 0, $false)
    pointerconst = @('int f(int *const a){return a[0];}int main(void){int a[1]={42};return f(a)!=42;}', 0, $false)
}
$cases['varmacro'] = @("#define V(x,...) ((x)+__VA_ARGS__)`nint main(void){return V(40,2)!=42;}", 1, $false)
$cases['emptymacro'] = @("#define V(...) 42`nint main(void){return V()!=42;}", 1, $false)
$cases['inactive'] = @("#if 0`n#define V(...) __VA_ARGS__`n#endif`nint main(void){return 0;}", 0, $false)
$cases['varfunction'] = @('int f(int n,...){return n;}int main(void){return f(42,20)!=42;}', 0, $false)
$cases['longlongtype'] = @('int main(void){long long n=42;return n!=42;}', 1, $false)
$cases['longlongsuffix'] = @('int main(void){return 42LL!=42;}', 1, $false)
$cases['wideinteger'] = @('int main(void){return 4294967296==0;}', 1, $false)
$cases['ordinarylong'] = @('int main(void){long n=42L;return n!=42;}', 0, $false)
$cases['ulongboundary'] = @('int main(void){return 0xffffffffUL!=4294967295UL;}', 0, $false)
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_OUTPUT = if ($ObjectMode) {
        'object'
    }
    else {
        $null
    }
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($g in 1, 2) {
        foreach ($mode in 'c89', 'c99', 'default') {
            $env:C1_STANDARD = if ($mode -eq 'default') {
                $null
            }
            else {
                $mode
            }
            $dir = Join-Path $BuildDirectory "$testRoot/$g/$mode"
            New-Item -ItemType Directory -Force $dir | Out-Null
            foreach ($name in $cases.Keys) {
                $source = Join-Path $dir "$name.c"
                $exe = Join-Path $dir "$name.exe"
                $err = Join-Path $dir "$name.stderr"
                $body = $cases[$name][0]
                if ($ObjectMode -and $name -eq 'errorafter') {
                    $body = 'int main(void){;int n=0;return n+;}' 
                }
                [IO.File]::WriteAllText($source, $body, [Text.Encoding]::ASCII)
                $artifact = if ($ObjectMode) {
                    Join-Path $dir "$name.c1o"
                }
                else {
                    $exe
                }
                $p = Start-Process -FilePath (Join-Path $BuildDirectory "host/c1-stage$g.exe") -WindowStyle Hidden -PassThru -RedirectStandardInput $source -RedirectStandardOutput $artifact -RedirectStandardError $err
                [void]$p.Handle
                if (!$p.WaitForExit(30000)) {
                    $p.Kill()
                    throw 'Compiler timeout'
                }
                $p.WaitForExit()
                $expected = if ($cases[$name][2]) {
                    1
                }
                else {
                    0
                }
                if ($p.ExitCode -ne $expected) {
                    throw "Unexpected compile result: $name $mode"
                }
                $diagnostics = Get-Content $err
                $count = if ($mode -eq 'c89') {
                    $cases[$name][1]
                }
                else {
                    0
                }
                $warnings = @($diagnostics | Where-Object { $_ -like 'C1 warning: C99 extension in C89 mode: *' }).Count
                if (($count -ge 0 -and $warnings -ne $count) -or ($count -lt 0 -and $warnings -lt - $count)) {
                    throw "Wrong warning count: $name $mode"
                }
                if ($expected) {
                    if (@($diagnostics | Where-Object { $_ -like 'C1 error:*' }).Count -ne 1) {
                        throw 'Warning suppressed the subsequent error'
                    }
                    if ((Get-Item $artifact).Length) {
                        throw 'Invalid source emitted binary bytes'
                    }
                }
                else {
                    if (@($diagnostics).Count -ne $warnings) {
                        throw 'Unexpected extra diagnostic'
                    }
                    if ($ObjectMode) {
                        $linker = (Join-Path $BuildDirectory "link-stage$g.exe")
                        $p = Start-Process -FilePath $linker -ArgumentList ('"' + $exe + '" main "' + $artifact + '"') -WindowStyle Hidden -PassThru
                        [void]$p.Handle
                        if (!$p.WaitForExit(30000)) {
                            $p.Kill()
                            throw 'Object link timeout'
                        }
                        $p.WaitForExit()
                        if ($p.ExitCode) {
                            throw 'Object link failed'
                        }
                    }
                    $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
                    [void]$p.Handle
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Execution timeout'
                    }
                    $p.WaitForExit()
                    if ($p.ExitCode) {
                        throw 'Execution failed'
                    }
                }
                "PASS generation $g $mode $name diagnostics and outcome"
            }
        }
    }
    $extensions = if ($ObjectMode) {
        @('exe', 'c1o')
    }
    else {
        @('exe')
    }
    foreach ($ext in $extensions) {
        foreach ($mode in 'c89', 'c99', 'default') {
            foreach ($name in ($cases.Keys | Where-Object { !$cases[$_][2] })) {
                $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "$testRoot/1/$mode/$name.$ext"))
                $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "$testRoot/2/$mode/$name.$ext"))
                if (!$a.Length -or $a.Length -ne $b.Length) {
                    throw 'Artifact length mismatch'
                }
                for ($i = 0; $i -lt $a.Length; $i++) {
                    if ($a[$i] -ne $b[$i]) {
                        throw 'Artifact mismatch'
                    }
                }
            }
        }
    }
    if ($ObjectMode) {
        'PASS 228 object diagnostic cases and 222 nonempty equal artifact pairs'
    }
    else {
        'PASS 228 diagnostic cases and 111 nonempty equal executable pairs'
    }
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
