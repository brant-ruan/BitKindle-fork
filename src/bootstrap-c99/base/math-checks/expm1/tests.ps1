
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$old = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'expm1-special-values' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;v.f=expm1(-0.0);if(v.u!=0x8000000000000000ULL||expm1(0)!=0||expm1(-HUGE_VAL)!=-1||expm1(HUGE_VAL)!=HUGE_VAL)return 1;v.u=0xfff0000000000022ULL;feclearexcept(FE_ALL_EXCEPT);v.f=expm1(v.f);return v.u!=0xfff8000000000022ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'expm1-subnormal-direction' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,r;x.u=1;fesetround(FE_UPWARD);errno=0;feclearexcept(FE_ALL_EXCEPT);r.f=expm1(x.f);if(r.u!=2||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_INEXACT|FE_UNDERFLOW))return 1;x.u=0x8000000000000001ULL;r.f=expm1(x.f);return r.u!=0x8000000000000000ULL;}') 0
    Check 'expm1-negative-saturation' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;fesetround(FE_UPWARD);v.f=expm1(-1000);if(v.u!=0xbfefffffffffffffULL)return 1;fesetround(FE_DOWNWARD);v.f=expm1(-1000);return v.u!=0xbff0000000000000ULL;}') 0
    Check 'expm1-environment' ($prefix + 'int main(void){fenv_t e;double x;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_DOWNWARD);feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);errno=73;x=expm1(0.1);fegetenv(&e);return x<=0||(e.__mxcsr&0x8040)!=0x8040||fegetround()!=FE_DOWNWARD||errno!=73||fetestexcept(FE_ALL_EXCEPT)!=(FE_INEXACT|FE_DIVBYZERO);}') 0
    . (Join-Path $PSScriptRoot 'oracle.ps1')
    foreach ($precision in 53, 24) {
        $inputs = @(-40, -10, -1, -0.5, -0.1, -1e-20, 1e-20, 1e-8, 0.1, 0.5, 1, 10, 36, 40)
        if ($precision -eq 53) {
            $inputs += @(709, 709.782712893384)
        }
        else {
            $inputs += @(88, 88.72)
        }
        $centers = if ($precision -eq 53) {
            @([C1Math]::ScaleB(1, -54), - [C1Math]::ScaleB(1, -54), 0.5, -0.5, -40, 709.782712893384)
        }
        else {
            @([C1Math]::ScaleB(1, -25), - [C1Math]::ScaleB(1, -25), 0.5, -0.5, -40, 88.722839)
        }
        foreach ($center in $centers) {
            if ($precision -eq 53) {
                $inputs += @([C1Math]::BitDecrement($center), $center, [C1Math]::BitIncrement($center))
            }
            else {
                $center = [float]$center
                $inputs += @([C1MathF]::BitDecrement($center), $center, [C1MathF]::BitIncrement($center))
            }
        }
        $suffix = if ($precision -eq 53) {
            'ULL'
        }
        else {
            'U'
        }
        $rows = foreach ($x in $inputs) {
            if ($precision -eq 24) {
                $x = [double][float]$x
            }
            foreach ($mode in 0..3) {
                $expected = ReferenceExpm1Bits $x $precision $mode
                $a = if ($precision -eq 53) {
                    [C1Bits]::DoubleToUInt64Bits($x)
                }
                else {
                    [C1Bits]::SingleToUInt32Bits([float]$x)
                }
                '{0x' + $a.ToString('X') + $suffix + ',0x' + $expected.ToString('X') + $suffix + ',' + $mode + '}'
            }
        }
        $type = if ($precision -eq 53) {
            'double'
        }
        else {
            'float'
        }
        $integer = if ($precision -eq 53) {
            'unsigned long long'
        }
        else {
            'unsigned int'
        }
        $fn = if ($precision -eq 53) {
            'expm1'
        }
        else {
            'expm1f'
        }
        $extra = if ($precision -eq 53) {
            'r.f=(double)expm1l((long double)x.f);if(r.u!=rows[i][1])return 2;'
        }
        else {
            ''
        }
        Check "expm1-high-precision-$precision" ($prefix + 'static ' + $integer + ' rows[][3]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,r;for(i=0;i<' + $rows.Count + ';i++){x.u=rows[i][0];fesetround(modes[rows[i][2]]);r.f=' + $fn + '(x.f);if(r.u!=rows[i][1])return 1;' + $extra + '}return 0;}') 0
    }
    . (Join-Path $PSScriptRoot 'random-tests.ps1')
}
finally {
    $env:C1_INCLUDE = $old
}
