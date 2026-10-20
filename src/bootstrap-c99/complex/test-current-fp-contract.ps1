param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
$build = $BuildDirectory
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'"))
$formats = @(
    @('float', '0x1.000002p0f', '0x1.fffffcp-1f', '1.0f'),
    @('double', '0x1.0000000000001p0', '0x1.ffffffffffffep-1', '1.0'),
    @('long double', '0x1.0000000000001p0L', '0x1.ffffffffffffep-1L', '1.0L')
)
$commands = @()
$names = @()
foreach ($switch in 'ABSENT', 'OFF', 'ON', 'DEFAULT') {
    foreach ($row in $formats) {
        $type = $row[0]
        $a = $row[1]
        $b = $row[2]
        $one = $row[3]
        $name = $switch + '-' + $type.Replace(' ', '-')
        $names += $name
        $directive = if ($switch -eq 'ABSENT') {
            ''
        }
        else {
            "#pragma STDC FP_CONTRACT $switch`n"
        }
        $body = $directive + "static $type folded=($a*$b)-$one;int main(void){$type a=$a,b=$b;return folded!=0||a*b-$one!=0;}"
        $commands += "Check '$name' '" + $body + "' 0"
    }
}
$nested = @'
#pragma STDC FP_CONTRACT OFF
int main(void){
 double a=0x1.0000000000001p0,b=0x1.ffffffffffffep-1;
 if(a*b-1.0!=0)return 1;
 {
#pragma STDC FP_CONTRACT ON
  if(a*b-1.0!=0)return 2;
  {
_Pragma("STDC FP_CONTRACT OFF")
   if(a*b-1.0!=0)return 3;
  }
 }
 return a*b-1.0!=0;
}
'@
$commands += "Check 'nested-restoration' '" + $nested + "' 0"
$names += 'nested-restoration'
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
        & ([scriptblock]::Create($prefix + ($commands -join "`n"))) -Compiler "$BuildDirectory/host/c1-stage$g.exe" -Assembler "$build/link-stage$g.exe" -CompilerKind C1 -ScratchDirectory "$PSScriptRoot/current-contract$g"
        if (!$?) {
            throw 'FP_CONTRACT test failed'
        }
    }
    foreach ($name in $names) {
        if ((Get-FileHash "$PSScriptRoot/current-contract1/$name.exe").Hash -ne (Get-FileHash "$PSScriptRoot/current-contract2/$name.exe").Hash) {
            throw 'Generation mismatch'
        }
    }
    'PASS 26 noncontracted floating executions and 13 equal executable pairs'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}

