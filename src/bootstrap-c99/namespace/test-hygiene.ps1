param([Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$AuditOnly)
$ErrorActionPreference = 'Stop'
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $snapshot 'tools/compile.ps1')
$cases = [ordered]@{
    'stdlib' = @('size count block text end base compare key dest value numer denom seed', 'div_t d;d=div(85,2);return d.quot!=42||atoi("42")!=42;')
    'ctype' = @('c', 'return !isdigit(48)||tolower(65)!=97;')
    'string' = @('dest src count left right data value text reject accept needle delimiters error_number', 'char b[4];strcpy(b,"42");return strcmp(b,"42")||strlen(b)!=2;')
    'locale' = @('category locale', 'return setlocale(LC_ALL,"C")==0||localeconv()->decimal_point[0]!=46;')
    'stdio' = @('handle active readable writable binary append eof error pushed lookahead buffer capacity readpos readend writeused owned buffering direction orientation offset', @'
FILE *f;fpos_t pos;char b[3];
f=tmpfile();if(!f)return 1;
if(fputs("abc",f)<0||fflush(f)||fseek(f,1,SEEK_SET))return 2;
if(fgetpos(f,&pos)||fgetc(f)!=98||fsetpos(f,&pos)||fgetc(f)!=98)return 3;
rewind(f);if(fread(b,1,3,f)!=3||b[0]!=97||b[2]!=99)return 4;
return fclose(f)!=0;
'@)
}
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    if (!$AuditOnly) {
        $options = ((Get-Content (Join-Path $snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
        $env:C1_RUNTIME = $null
        $env:C1_OUTPUT = 'object'
        $env:C1_INCLUDE = ((@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
        foreach ($g in 1, 2) {
            foreach ($mode in 'c89', 'c99', 'default') {
                $env:C1_STANDARD = if ($mode -eq 'default') {
                    $null
                }
                else {
                    $mode
                }
                $dir = Join-Path $BuildDirectory "hygiene/$g/$mode"
                New-Item -ItemType Directory -Force $dir | Out-Null
                foreach ($kind in $cases.Keys) {
                    $source = Join-Path $dir "$kind.c"
                    $obj = Join-Path $dir "$kind.c1o"
                    $exe = Join-Path $dir "$kind.exe"
                    $body = (($cases[$kind][0].Split(' ') | ForEach-Object { "#define $_ +" }) -join "`n") + "`n#include <$kind.h>`nint main(void){" + $cases[$kind][1] + '}'
                    [IO.File]::WriteAllText($source, $body, [Text.Encoding]::ASCII)
                    Compile (Join-Path $BuildDirectory "host/c1-stage$g.exe") $source $obj
                    $runtime = if ($mode -eq 'c89') {
                        Join-Path $BuildDirectory "namespace/runtime-policy$g/runtime.c1o"
                    }
                    else {
                        Join-Path $BuildDirectory "host/runtime$g.c1o"
                    }
                    $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList ('"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options) -WindowStyle Hidden -PassThru
                    [void]$p.Handle
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Hygiene link timeout'
                    }
                    $p.WaitForExit()
                    if ($p.ExitCode) {
                        throw 'Hygiene link failed'
                    }
                    $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
                    [void]$p.Handle
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Hygiene execution timeout'
                    }
                    $p.WaitForExit()
                    if ($p.ExitCode) {
                        throw "Hygiene $kind returned $($p.ExitCode)"
                    }
                    "PASS hygiene generation $g $mode $kind"
                }
            }
        }
    }
    foreach ($mode in 'c89', 'c99', 'default') {
        foreach ($kind in $cases.Keys) {
            foreach ($ext in 'c1o', 'exe') {
                $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "hygiene/1/$mode/$kind.$ext"))
                $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "hygiene/2/$mode/$kind.$ext"))
                if (!$a.Length -or $a.Length -ne $b.Length) {
                    throw 'Hygiene artifact length mismatch'
                }
                for ($i = 0; $i -lt $a.Length; $i++) {
                    if ($a[$i] -ne $b[$i]) {
                        throw 'Hygiene artifact byte mismatch'
                    }
                }
            }
        }
    }
    if ($AuditOnly) {
        $log = Get-Content (Join-Path $BuildDirectory 'HYGIENE.txt')
        foreach ($g in 1, 2) {
            foreach ($mode in 'c89', 'c99', 'default') {
                foreach ($kind in $cases.Keys) {
                    if (@($log | Where-Object { $_ -eq "PASS hygiene generation $g $mode $kind" }).Count -ne 1) {
                        throw 'Missing unique hygiene execution gate'
                    }
                }
            }
        }
        if ($log[-1] -ne 'PASS header hygiene: 30 client executions and 30 identical artifact pairs') {
            throw 'Missing hygiene final gate'
        }
    }
    'PASS header hygiene: 30 client executions and 30 identical artifact pairs'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
