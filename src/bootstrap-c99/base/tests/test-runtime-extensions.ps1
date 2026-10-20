$savedExtendedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$extendedRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).Replace('\', '/') + '/'
function ExtendedCheck([string]$Name, [string]$Source, [int]$Result = 0, [bool]$Reject = $false, [string]$RuntimeInput = '', [string]$ExpectedOutput = '') {
    Check $Name ($Source.Replace('src/bootstrap-c99/base/', $extendedRoot)) $Result $Reject $RuntimeInput $ExpectedOutput
}
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    foreach ($part in @('adjacent', 'order', 'rounding', 'strtofloat', 'setjmp')) {
        . (Join-Path $PSScriptRoot ('test-extended-' + $part + '.ps1'))
    }
    ExtendedCheck 'extended-library-header-integration' @"
#include <stdlib.h>
#include <math.h>
#include <setjmp.h>
#include <fenv.h>
#include <setjmp.h>
jmp_buf env;int attempts;
void transfer(void){if(strtod("1.5",(char **)0)==1.5)longjmp(env,7);}
int main(void){
 if(7==setjmp(env)){
  fesetround(FE_TONEAREST);
  if(lrint(atof("1.5"))!=2||fmax(1.0,nexttoward(1.0,2.0L))<=1.0)return 1;
  return sizeof(jmp_buf)!=256;
 }
 if(attempts++)return 2;
 transfer();return 3;
}
#include "src/bootstrap-c99/base/lib/runtime.c"
"@
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedExtendedInclude, 'Process')
}