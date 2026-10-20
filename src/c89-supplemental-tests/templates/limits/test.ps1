param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$CompilerDirectory = Join-Path $build 'host'
$ErrorActionPreference = 'Stop'
. (Join-Path $Snapshot 'tools/compile.ps1')
# Generate C source only. Every executable is emitted by the self-built compiler.
$text = [Text.StringBuilder]::new()
$headerDir = Join-Path $PSScriptRoot 'limit-headers'
New-Item -ItemType Directory -Force $headerDir | Out-Null
foreach ($i in 1..8) {
    $header = if ($i -eq 8) {
        '#define INCLUDED_EIGHT 42'
    }
    else {
        '#include "level' + ($i + 1) + '.h"'
    }
    [IO.File]::WriteAllText((Join-Path $headerDir "level$i.h"), $header, [Text.Encoding]::ASCII)
}
[void]$text.AppendLine('#include "' + ([IO.Path]::GetFullPath((Join-Path $headerDir 'level1.h')).Replace('\', '/')) + '"')
[void]$text.AppendLine('#define SUM31(' + ((1..31 | ForEach-Object { "p$_" }) -join ',') + ') (' + ((1..31 | ForEach-Object { "(p$_)" }) -join '+') + ')')
foreach ($i in 1..511) {
    [void]$text.AppendLine("#define M$i $i")
}
foreach ($i in 1..8) {
    [void]$text.AppendLine('#if 1')
}
foreach ($i in 1..511) {
    [void]$text.AppendLine("int g$i;")
}
foreach ($i in 1..8) {
    [void]$text.AppendLine('#endif')
}
[void]$text.AppendLine('struct S{' + ((1..127 | ForEach-Object { "int member$_;" }) -join "`n") + '};')
[void]$text.AppendLine('union U{' + ((1..127 | ForEach-Object { "int member$_;" }) -join "`n") + '};')
[void]$text.AppendLine('enum E{' + ((1..127 | ForEach-Object { "enum$_=$_" }) -join ",`n") + '};')
[void]$text.AppendLine(((1..15 | ForEach-Object { "struct Nested$_ {" }) -join '') + 'int value;' + ('} child;' * 14) + '};')
[void]$text.AppendLine('int ************deep;')
[void]$text.AppendLine('int ' + ('(' * 31) + 'nested' + (')' * 31) + ';')
[void]$text.AppendLine('char large[32767];')
[void]$text.AppendLine('char literal[]="' + ('a' * 255) + '"' + "`n" + '"' + ('a' * 254) + '";')
[void]$text.AppendLine('static int nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxa=20,nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxb=22;')
[void]$text.AppendLine('int exterA=20,exterB=22;')
[void]$text.AppendLine('int parameters(' + ((1..31 | ForEach-Object { "int arg$_" }) -join ',') + '){return ' + ((1..31 | ForEach-Object { "arg$_" }) -join '+') + ';}')
[void]$text.AppendLine('int main(void){struct S s;union U u;')
[void]$text.AppendLine(((1..127 | ForEach-Object { "int local$_=$_;" }) -join "`n"))
[void]$text.AppendLine('s.member127=20;u.member127=22;g1=1;g511=511;large[32766]=42;')
[void]$text.AppendLine('if(parameters(' + ((1..31) -join ',') + ')!=496)return 1;')
[void]$text.AppendLine('if(SUM31(' + ((1..31) -join ',') + ')!=496||INCLUDED_EIGHT!=42||sizeof(struct Nested1)!=4)return 9;')
[void]$text.AppendLine('if(s.member127+u.member127!=42||local127!=127||enum127!=127)return 2;')
[void]$text.AppendLine('if(g1!=1||g511!=511||M1!=1||M511!=511)return 3;')
[void]$text.AppendLine('if(sizeof(large)!=32767||large[32766]!=42||sizeof(literal)!=510)return 4;')
[void]$text.AppendLine('if(nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxa+nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxb!=42)return 5;')
[void]$text.AppendLine('switch(local127){' + ((1..257 | ForEach-Object { "case ${_}:if(local127!=$_)return 6;break;" }) -join "`n") + 'default:return 7;}')
[void]$text.AppendLine(('{' * 31) + 'if(' + ('(' * 32) + '42' + (')' * 32) + '!=42)return 8;' + ('}' * 31))
[void]$text.AppendLine('if(exterA+exterB!=42)return 10;return 0;}')
foreach ($line in ($text.ToString() -split "`n")) {
    if ($line.TrimEnd("`r").Length -gt 509) {
        throw 'Probe exceeds 509-character logical line'
    }
}
$names = @(1..511 | ForEach-Object { "g$_" }) + @('deep', 'nested', 'large', 'literal', 'exterA', 'exterB', 'parameters', 'main')
$keys = @{}
foreach ($name in $names) {
    $key = $name.Substring(0, [Math]::Min(6, $name.Length)).ToLowerInvariant()
    if ($keys.ContainsKey($key)) {
        throw 'External first-six-character collision'
    }
    $keys[$key] = $true
}
'PASS probe external names unique within six characters and logical lines at most 509 characters'
$source = Join-Path $PSScriptRoot 'translation-limits.c'
[IO.File]::WriteAllText($source, $text.ToString(), [Text.Encoding]::ASCII)
$saved = $env:C1_OUTPUT
$savedStandard = $env:C1_STANDARD
$savedRuntime = $env:C1_RUNTIME
$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = $null
    $env:C1_OUTPUT = $null
    foreach ($g in 1, 2) {
        $dir = Join-Path $PSScriptRoot "limits$g"
        New-Item -ItemType Directory -Force $dir | Out-Null
        $exe = Join-Path $dir 'limits.exe'
        Compile ([IO.Path]::GetFullPath("$CompilerDirectory/c1-stage$g.exe")) $source $exe
        $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Limits execution timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw "Combined limits failed: $($p.ExitCode)"
        }
        "PASS generation $g combined translation limits"
    }
    $a = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'limits1/limits.exe'))
    $b = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'limits2/limits.exe'))
    if (!$a.Length -or $a.Length -ne $b.Length) {
        throw 'Limits executable length mismatch'
    }
    for ($i = 0; $i -lt $a.Length; $i++) {
        if ($a[$i] -ne $b[$i]) {
            throw 'Limits executable bytes differ'
        }
    }
    'PASS combined limits executables identical'
}
finally {
    $env:C1_OUTPUT = $saved
    $env:C1_STANDARD = $savedStandard
    $env:C1_RUNTIME = $savedRuntime
    $env:C1_INCLUDE = $savedInclude
}
