$savedInclude = $env:C1_INCLUDE
$savedTmp = $env:TMP
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include'))
    $env:TMP = $ScratchDirectory
    foreach ($file in 'test-stdio-default.ps1', 'test-stdio-wide.ps1', 'test-stdio-scan-float.ps1', 'test-stdio-temporary.ps1', 'test-stdio-temporary-limit.ps1', 'test-stdio-integer-oracles.ps1', 'test-stdio-decimal-oracles.ps1', 'test-stdio-hex-oracles.ps1', 'test-stdio-scan-oracles.ps1', 'test-stdio-lifecycle.ps1', 'test-stdio-platform.ps1') {
        . (Join-Path $PSScriptRoot $file)
    }
    Check 'stdio-public-header-closure' @'
#include <stdio.h>
#include <stdlib.h>
static void done(void){printf("done");}
int main(void){char b[32];int x;if(sscanf("17","%d",&x)!=1)return 1;
if(snprintf(b,sizeof(b),"%d",x)!=2||atexit(done))return 2;return 0;}
#include "src/bootstrap-c99/base/lib/runtime.c"
'@ 0 $false '' 'done'
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $env:TMP = $savedTmp
}
