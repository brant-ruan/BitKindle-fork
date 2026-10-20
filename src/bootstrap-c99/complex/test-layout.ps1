param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'")).Replace("'^C1 error: '", "'(?m)^C1 error: '")
$positive = [ordered]@{
    sizes = 'int main(void){return sizeof(float _Complex)!=2*sizeof(float)||sizeof(double _Complex)!=2*sizeof(double)||sizeof(long double _Complex)!=2*sizeof(long double);}'
    order = 'int main(void){return sizeof(_Complex float)!=8||sizeof(_Complex long double)!=16||sizeof(long _Complex double)!=16;}'
    aliases = 'typedef double _Complex Z;int main(void){const Z a[3];Z *p;return sizeof a!=48||sizeof p!=8;}'
    members = 'struct S {char c;double _Complex z;char d;};int main(void){return sizeof(struct S)!=32;}'
    vla = 'int main(void){int n=3;double _Complex a[n];return sizeof a!=48;}'
    pointers = 'int main(void){double _Complex a[3];return (&a[2]-&a[0])!=2;}'
}
$negative = [ordered]@{
    integer = 'int _Complex x;int main(void){return 0;}'
    duplicate = 'double _Complex _Complex x;int main(void){return 0;}'
    bare = '_Complex x;int main(void){return 0;}'
    unsigned = 'unsigned double _Complex x;int main(void){return 0;}'
    incompatible = 'float _Complex *p;double _Complex *p;int main(void){return 0;}'
}
$commands = @()
foreach ($n in $positive.Keys) {
    $commands += "Check '$n' '$($positive[$n])' 0"
}
foreach ($n in $negative.Keys) {
    $commands += "Check 'reject-$n' '$($negative[$n])' 0 `$true"
}
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($g in 1, 2) {
        & ([scriptblock]::Create($prefix + ($commands -join "`n"))) -Compiler "$BuildDirectory/host/c1-stage$g.exe" -Assembler "$BuildDirectory/link-stage$g.exe" -CompilerKind C1 -ScratchDirectory "$PSScriptRoot/layout$g"
        if (!$?) {
            throw 'Complex layout failed'
        }
    }
    foreach ($n in $positive.Keys) {
        if ((Get-FileHash "$PSScriptRoot/layout1/$n.exe").Hash -ne (Get-FileHash "$PSScriptRoot/layout2/$n.exe").Hash) {
            throw 'Complex generation mismatch'
        }
    }
    'PASS 12 complex layout executions, ten constraint rejections and six equal pairs'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
