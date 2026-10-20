param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$SourceRoot = $PSScriptRoot
$SnapshotRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$CompilerDirectory = Join-Path $BuildDirectory 'host'
$OutputDirectory = Join-Path $BuildDirectory 'namespace'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$ErrorActionPreference = 'Stop'
. (Join-Path $SnapshotRoot 'tools/compile.ps1')
$savedStandard = $env:C1_STANDARD
$savedInclude = $env:C1_INCLUDE
$savedOutput = $env:C1_OUTPUT
$savedRuntime = $env:C1_RUNTIME
$client = @'
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
int snprintf=20,atoll=22;
double round=42.0;
int main(void){
 char b[16];int *p=malloc(sizeof(int));
 if(!p)return 1;*p=snprintf+atoll;free(p);
 if(snprintf+atoll!=42||round!=42.0)return 2;
 if(sprintf(b,"%d",42)!=2||b[0]!='4'||b[1]!='2')return 3;
 if(floor(42.5)!=42.0)return 4;
 return 0;
}
'@
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/components/wide-input') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/components/wide-io') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/components/wctype') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $apis = Get-Content src/bootstrap-c99/runtime/imports.json -Raw | ConvertFrom-Json
    $options = ($apis | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        $dir = Join-Path $OutputDirectory "runtime-policy$g"
        New-Item -ItemType Directory -Force $dir | Out-Null
        $compiler = Join-Path $CompilerDirectory "c1-stage$g.exe"
        $env:C1_STANDARD = $null
        $env:C1_OUTPUT = 'object'
        $env:C1_RUNTIME = 'c89'
        $runtime = Join-Path $dir 'runtime.c1o'
        Compile $compiler ([IO.Path]::GetFullPath('src/bootstrap-c99/runtime/runtime-c89.c')) $runtime
        $env:C1_RUNTIME = $null
        $env:C1_STANDARD = 'c89'
        foreach ($kind in 'collision', 'addresses') {
            $source = Join-Path $dir "$kind.c"
            $body = if ($kind -eq 'collision') {
                $client
            }
            else {
                Get-Content (Join-Path $SourceRoot 'library-addresses.c') -Raw
            }
            [IO.File]::WriteAllText($source, $body, [Text.Encoding]::ASCII)
            $obj = Join-Path $dir "$kind.c1o"
            $exe = Join-Path $dir "$kind.exe"
            Compile $compiler $source $obj
            $line = '"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options
            $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList $line -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Runtime namespace link timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw 'Runtime namespace link failed'
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Runtime namespace execution timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Runtime namespace $kind failed: $($p.ExitCode)"
            }
            "PASS generation $g runtime namespace $kind"
        }
    }
    foreach ($name in 'runtime.c1o', 'collision.c1o', 'collision.exe', 'addresses.c1o', 'addresses.exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "runtime-policy1/$name"))
        $b = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "runtime-policy2/$name"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw "Unequal length $name"
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Unequal bytes $name"
            }
        }
        "PASS identical $name"
    }
}
finally {
    $env:C1_STANDARD = $savedStandard
    $env:C1_INCLUDE = $savedInclude
    $env:C1_OUTPUT = $savedOutput
    $env:C1_RUNTIME = $savedRuntime
}
