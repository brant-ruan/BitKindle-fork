param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'

$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
. (Join-Path $snapshot 'tools/compile.ps1')
$build = [IO.Path]::GetFullPath($BuildDirectory)
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
$fields = 'int_p_cs_precedes int_p_sep_by_space int_n_cs_precedes int_n_sep_by_space int_p_sign_posn int_n_sign_posn'.Split(' ')
$collision = (($fields | ForEach-Object { "#define $_ +" }) -join "`n") + "`n"
$client = @'
#include <locale.h>
#include <limits.h>
#include <string.h>
int main(void){
 struct lconv *p;
 if(!setlocale(LC_ALL,"C"))return 1;
 p=localeconv();
 if(strcmp(p->decimal_point,".")||p->thousands_sep[0]||p->grouping[0])return 2;
 if(p->int_curr_symbol[0]||p->currency_symbol[0]||p->mon_decimal_point[0])return 3;
 if(p->mon_thousands_sep[0]||p->mon_grouping[0]||p->positive_sign[0]||p->negative_sign[0])return 4;
 if(p->int_frac_digits!=CHAR_MAX||p->frac_digits!=CHAR_MAX)return 5;
 if(p->p_cs_precedes!=CHAR_MAX||p->p_sep_by_space!=CHAR_MAX||p->n_cs_precedes!=CHAR_MAX||p->n_sep_by_space!=CHAR_MAX)return 6;
 if(p->p_sign_posn!=CHAR_MAX||p->n_sign_posn!=CHAR_MAX)return 7;
#if !defined(__C1_C89__)
 if(p->int_p_cs_precedes!=CHAR_MAX||p->int_p_sep_by_space!=CHAR_MAX||p->int_n_cs_precedes!=CHAR_MAX)return 8;
 if(p->int_n_sep_by_space!=CHAR_MAX||p->int_p_sign_posn!=CHAR_MAX||p->int_n_sign_posn!=CHAR_MAX)return 9;
#endif
 return 0;
}
'@
try {
    $options = ((Get-Content (Join-Path $snapshot 'runtime/imports.json') -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    $baseInclude = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';'
    foreach ($g in 1, 2) {
        $dir = Join-Path $build "namespace/locale$g"
        New-Item -ItemType Directory -Force $dir | Out-Null
        $compiler = Join-Path $build "host/c1-stage$g.exe"
        foreach ($mode in 'c89', 'c99') {
            $source = Join-Path $dir "$mode.c"
            $text = if ($mode -eq 'c89') {
                $collision + $client
            }
            else {
                $client
            }
            [IO.File]::WriteAllText($source, $text, [Text.Encoding]::ASCII)
            $env:C1_OUTPUT = 'object'
            $env:C1_STANDARD = $mode
            $env:C1_RUNTIME = $null
            $env:C1_INCLUDE = (Join-Path $snapshot 'base/include') + ';' + $baseInclude
            if ($mode -eq 'c89') {
                $rejected = $false
                $control = Join-Path $dir 'original.c1o'
                try {
                    Compile $compiler $source $control
                }
                catch {
                    if ($_.Exception.Message -notmatch 'Compile failed \(1\)') {
                        throw
                    }
                    $rejected = $true
                }
                if (!$rejected -or (Get-Item $control).Length) {
                    throw 'Original locale collision was not rejected without output'
                }
                "PASS generation $g original C89 locale member collision reproduced"
            }
            $env:C1_INCLUDE = (Join-Path $snapshot 'include') + ';' + $baseInclude
            $obj = Join-Path $dir "$mode.c1o"
            $exe = Join-Path $dir "$mode.exe"
            Compile $compiler $source $obj
            $env:C1_STANDARD = $null
            $env:C1_RUNTIME = if ($mode -eq 'c89') {
                'c89'
            }
            else {
                $null
            }
            $runtime = Join-Path $dir "runtime-$mode.c1o"
            Compile $compiler (Join-Path $snapshot "runtime/runtime-$mode.c") $runtime
            $p = Start-Process -FilePath (Join-Path $build "link-stage$g.exe") -ArgumentList ('"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options) -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Link timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw 'Link failed'
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Run timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Locale returned $($p.ExitCode)"
            }
            "PASS generation $g $mode locale members and values"
        }
    }
    foreach ($mode in 'c89', 'c99') {
        foreach ($name in "runtime-$mode.c1o", "$mode.c1o", "$mode.exe") {
            $a = [IO.File]::ReadAllBytes((Join-Path $build "namespace/locale1/$name"))
            $b = [IO.File]::ReadAllBytes((Join-Path $build "namespace/locale2/$name"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Unequal artifact lengths'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Unequal artifact bytes'
                }
            }
            "PASS identical $name"
        }
    }
    'PASS C89 locale namespace and C99 members, four clients and six artifact pairs'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
