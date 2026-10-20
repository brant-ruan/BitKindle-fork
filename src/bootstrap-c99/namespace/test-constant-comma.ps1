param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$Original = $false
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'")).Replace("'^C1 error: '", "'(?m)^C1 error: '")
$cases = @(
    @{name = 'enum'; source = 'enum{A=1?3:(4,5)};int main(void){return A!=3;}'; warning = $true },
    @{name = 'static'; source = 'int a=1||(2,3);int main(void){return a!=1;}'; warning = $true },
    @{name = 'pointer-static'; source = 'int*p=1?0:(1,2);int main(void){return p!=0;}'; warning = $true },
    @{name = 'pointer-auto'; source = 'int main(void){int*p=1?0:(1,2);return p!=0;}'; reject89 = $true },
    @{name = 'pointer-call'; source = 'int f(int*p){return p!=0;}int main(void){return f(0&&(1,2));}'; reject89 = $true },
    @{name = 'sizeof'; source = 'enum{A=sizeof((1,2))};int main(void){int*p=sizeof((1,2))-sizeof(int);return A!=sizeof(int)||p!=0;}' },
    @{name = 'runtime'; source = 'int main(void){int i=0;i=(1,2);return i!=2;}' },
    @{name = 'short-overflow'; source = 'enum{A=1?3:(2147483647+1)};int main(void){return A!=3;}' },
    @{name = 'evaluated'; source = 'enum{A=(1,2)};int main(void){return 0;}'; reject = $true }
)
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    $tag = if ($Original) {
        'original'
    }
    else {
        'fixed'
    }
    foreach ($mode in 'c89', 'c99') {
        $env:C1_STANDARD = $mode
        foreach ($g in 1, 2) {
            $scratch = Join-Path $BuildDirectory "namespace/constant-comma/$tag-$mode-$g"
            $commands = foreach ($c in $cases) {
                $reject = $c.reject -or (!$Original -and $mode -eq 'c89' -and $c.reject89)
                "Check '$($c.name)' '" + $c.source.Replace("'", "''") + "' 0 " + $(if ($reject) {
                        '$true'
                    }
                    else {
                        '$false'
                    })
            }
            $compiler = if ($Original) {
                "$snapshot/build-precheck/host/c1-stage$g.exe"
            }
            else {
                Join-Path $BuildDirectory "host/c1-stage$g.exe"
            }
            & ([scriptblock]::Create($prefix + ($commands -join "`n"))) -Compiler $compiler -Assembler "$BuildDirectory/link-stage2.exe" -CompilerKind C1 -ScratchDirectory $scratch
            if (!$?) {
                throw 'Comma cases failed'
            }
            foreach ($c in $cases) {
                $err = Get-Content (Join-Path $scratch "$($c.name).err") -Raw
                $want = !$Original -and $mode -eq 'c89' -and $c.warning
                $found = $err -match 'C99 extension in C89 mode: unevaluated comma in constant expression'
                if ([bool]$found -ne [bool]$want) {
                    throw "Wrong diagnostic: $mode/$g/$($c.name)"
                }
            }
        }
    }
    'PASS 36 comma mode and diagnostic checks'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
