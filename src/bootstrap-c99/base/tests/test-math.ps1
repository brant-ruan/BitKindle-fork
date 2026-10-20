$savedMathInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$mathFixtureRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).Replace('\', '/') + '/'
function MathCheck([string]$Name, [string]$Source, [int]$Result = 0, [bool]$Reject = $false, [string]$RuntimeInput = '', [string]$ExpectedOutput = '') {
    Check $Name ($Source.Replace('src/bootstrap-c99/base/', $mathFixtureRoot)) $Result $Reject $RuntimeInput $ExpectedOutput
}
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    foreach ($part in @('basic', 'oracle', 'scale', 'scale-oracle', 'remainder', 'sqrt', 'classify', 'compare', 'compare-oracle', 'constants')) {
        . (Join-Path $PSScriptRoot ('test-math-' + $part + '.ps1'))
    }
    MathCheck 'math-scale-environment' ([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'math-scale-environment.c')))
    MathCheck 'math-foundation-combined' ('#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10 + 'int main(void){fenv_t saved;feholdexcept(&saved);errno=123;if(!isless(fmod(5.5,2),sqrt(4)))return 1;if(fpclassify(ldexpf(1.0f,-149))!=FP_SUBNORMAL)return 2;if(errno!=123||fetestexcept(FE_ALL_EXCEPT))return 3;fesetenv(&saved);return 0;}')
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedMathInclude, 'Process')
}