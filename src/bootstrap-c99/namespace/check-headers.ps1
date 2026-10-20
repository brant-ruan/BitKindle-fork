param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$SourceRoot = $PSScriptRoot
$SnapshotRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$CompilerDirectory = Join-Path $BuildDirectory 'host'
$OutputDirectory = Join-Path $BuildDirectory 'namespace'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null

$ErrorActionPreference = 'Stop'
. (Join-Path $SnapshotRoot 'tools/compile.ps1')
$cases = [ordered]@{
    'assert' = 'assert(1);return 0;'
    'ctype' = 'int (*p)(int)=isdigit;return sizeof(p)!=8;'
    'errno' = 'return EDOM==ERANGE;'
    'float' = 'return FLT_RADIX!=2||FLT_MANT_DIG<6||DBL_DIG<10||LDBL_DIG<10;'
    'limits' = 'return CHAR_BIT<8||INT_MAX<32767||LONG_MAX<2147483647L||MB_LEN_MAX<1;'
    'locale' = 'struct lconv x;return sizeof(x.decimal_point)!=sizeof(char *)||LC_ALL==LC_NUMERIC;'
    'math' = 'double x=HUGE_VAL;return x<=0.0;'
    'setjmp' = 'jmp_buf x;return sizeof(x)==0;'
    'signal' = 'sig_atomic_t x=0;return x||SIGABRT==SIGTERM||SIG_DFL==SIG_IGN;'
    'stdarg' = 'va_list x;return sizeof(x)==0;'
    'stddef' = 'struct S{char x;int y;};ptrdiff_t d=0;size_t n=sizeof(wchar_t);return d||!n||offsetof(struct S,y)==0||NULL!=0;'
    'stdio' = 'FILE *f=NULL;fpos_t pos;size_t n=sizeof(pos);return f!=0||!n||EOF>=0||BUFSIZ<256||FOPEN_MAX<8||FILENAME_MAX<1||L_tmpnam<1;'
    'stdlib' = 'div_t d;ldiv_t l;size_t n=sizeof(wchar_t);return !n||sizeof(d.quot)!=sizeof(int)||sizeof(l.rem)!=sizeof(long)||RAND_MAX<32767||MB_CUR_MAX<1;'
    'string' = 'size_t n=0;void *p=NULL;return n||p!=0;'
    'time' = 'clock_t c=0;time_t t=0;size_t n=sizeof(struct tm);return c||t||!n||CLOCKS_PER_SEC<1;'
}
$savedInclude = $env:C1_INCLUDE
$savedOutput = $env:C1_OUTPUT
$savedStandard = $env:C1_STANDARD
$savedRuntime = $env:C1_RUNTIME
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_RUNTIME = $null
    $snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
    $build = [IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($CompilerDirectory))
    $apis = Get-Content (Join-Path $snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json
    $options = ($apis | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/include') + ';' + ((@('components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $env:C1_OUTPUT = $null
    foreach ($g in 1, 2) {
        $dir = Join-Path $OutputDirectory "headers$g"
        New-Item -ItemType Directory -Force $dir | Out-Null
        foreach ($name in $cases.Keys) {
            $source = Join-Path $dir "$name.c"
            $exe = Join-Path $dir "$name.exe"
            # Include twice to verify re-inclusion, without any supporting standard header.
            [IO.File]::WriteAllText($source, ("#include <$name.h>`n#include <$name.h>`nint main(void){" + $cases[$name] + '}'), [Text.Encoding]::ASCII)
            $obj = Join-Path $dir "$name.c1o"
            $env:C1_OUTPUT = 'object'
            Compile ([IO.Path]::GetFullPath("$CompilerDirectory/c1-stage$g.exe")) $source $obj
            $env:C1_OUTPUT = $null
            $runtime = Join-Path $OutputDirectory "runtime-policy$g/runtime.c1o"
            if (!(Test-Path $runtime)) {
                throw 'Run test-runtime-policy.ps1 first to build the isolated C89 runtime'
            }
            $line = '"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options
            $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList $line -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Header link timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Header $name link failed"
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(10000)) {
                $p.Kill()
                throw 'Header execution timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Header $name returned $($p.ExitCode)"
            }
            "PASS generation $g isolated header $name"
        }
    }
    foreach ($name in $cases.Keys) {
        foreach ($ext in 'c1o', 'exe') {
            $a = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "headers1/$name.$ext"))
            $b = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "headers2/$name.$ext"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw "Header artifact lengths differ: $name.$ext"
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw "Header artifact bytes differ: $name.$ext"
                }
            }
            "PASS identical header artifact $name.$ext"
        }
    }
    'PASS 15 isolated C89 headers in both generations with isolated C89 runtime and 30 identical artifact pairs'
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $env:C1_OUTPUT = $savedOutput
    $env:C1_STANDARD = $savedStandard
    $env:C1_RUNTIME = $savedRuntime
}
