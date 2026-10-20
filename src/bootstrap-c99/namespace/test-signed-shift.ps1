param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$Fixed = $true
$testRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/signed-shift'
New-Item -ItemType Directory -Force $testRoot | Out-Null
$toolBuild = [IO.Path]::GetFullPath($BuildDirectory)
if ($ToolDirectory) {
    $toolBuild = [IO.Path]::GetFullPath($ToolDirectory)
}
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Get-Content (Join-Path $snapshot 'base/tests/test-c0.ps1') -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'"))
$prefix = $prefix.Replace("'^C1 error: '", "'(?m)^C1 error: '")
$cases = @(
    @{name = 'static-negative'; source = 'int x=-1<<1;int main(void){return x!=-2;}' },
    @{name = 'static-sign-bit'; source = 'int x=1<<31;int main(void){return x!=(-2147483647-1);}' },
    @{name = 'static-discard-high'; source = 'int x=-1<<31;int main(void){return x!=(-2147483647-1);}' },
    @{name = 'enum-negative'; source = 'enum E{X=-1<<1};int main(void){return X!=-2;}' },
    @{name = 'case-negative'; source = 'int main(void){switch(-2){case -1<<1:return 0;}return 1;}' },
    @{name = 'bound-negative'; source = 'int a[((-1<<1)==-2)?1:-1];int main(void){return sizeof(a)!=sizeof(int);}' },
    @{name = 'null-pointer'; source = 'int main(void){int *p=(-1<<1)+2;return p!=0;}' },
    @{name = 'preprocessor-negative'; source = "#if (-1<<1)!=-2`n#error bad`n#endif`nint main(void){return 0;}" },
    @{name = 'preprocessor-sign'; source = "#if (1L<<63)!=(-9223372036854775807L-1)`n#error bad`n#endif`nint main(void){return 0;}" },
    @{name = 'unsigned-control'; source = 'unsigned int x=1U<<31;int main(void){return x!=2147483648U;}'; control = $true },
    @{name = 'signed-control'; source = 'int x=1<<30;int main(void){return x!=1073741824;}'; control = $true },
    @{name = 'reject-negative-count'; source = 'int x=1<<-1;int main(void){return 0;}'; reject = $true },
    @{name = 'reject-wide-count'; source = 'int x=1<<32;int main(void){return 0;}'; reject = $true }
)
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($mode in 'c89', 'c99') {
        $commands = foreach ($c in $cases) {
            $reject = $c.reject -or (!$c.control -and (!$Fixed -or $mode -eq 'c99'))
            "Check '$($c.name)' '" + $c.source.Replace("'", "''") + "' 0 " + $(if ($reject) {
                    '$true'
                }
                else {
                    '$false'
                })
        }
        $selected = $prefix + ($commands -join "`n") + "`n" + '"PASS $script:passed signed shift cases"'
        $tag = if ($Fixed) {
            'fixed'
        }
        else {
            'original'
        }
        [IO.File]::WriteAllText((Join-Path $testRoot "$tag-$mode-selected.ps1"), $selected)
        foreach ($g in 1, 2) {
            $env:C1_STANDARD = $mode
            $compiler = Join-Path $toolBuild "host/c1-stage$g.exe"
            & ([scriptblock]::Create($selected)) -Compiler $compiler -Assembler "$toolBuild/link-stage2.exe" -CompilerKind C1 -ScratchDirectory (Join-Path $testRoot "$tag-$mode-$g")
            if (!$?) {
                throw 'Shift suite failed'
            }
        }
    }
    $pairs = 0
    foreach ($mode in 'c89', 'c99') {
        foreach ($c in $cases) {
            $reject = $c.reject -or (!$c.control -and (!$Fixed -or $mode -eq 'c99'))
            if (!$reject) {
                $a = [IO.File]::ReadAllBytes((Join-Path $testRoot "$tag-$mode-1/$($c.name).exe"))
                $b = [IO.File]::ReadAllBytes((Join-Path $testRoot "$tag-$mode-2/$($c.name).exe"))
                if (!$a.Length -or $a.Length -ne $b.Length) {
                    throw 'Unequal lengths'
                }
                for ($i = 0; $i -lt $a.Length; $i++) {
                    if ($a[$i] -ne $b[$i]) {
                        throw 'Different bytes'
                    }
                }
                $pairs++
            }
        }
    }
    "PASS 13 shift cases in both modes and generations; $pairs equal executable pairs"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
