
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$old = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'pow-special-values' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;v.f=pow(-0.0,3);if(v.u!=0x8000000000000000ULL)return 1;v.f=pow(-0.0,-3);if(v.u!=0xfff0000000000000ULL)return 2;if(pow(-1,HUGE_VAL)!=1||pow(HUGE_VAL,0)!=1||pow(-HUGE_VAL,-3)!=0)return 3;errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=pow(-2,0.1);return !isnan(v.f)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'pow-exact-integers-and-roots' ($prefix + 'int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;for(i=0;i<4;i++){fesetround(modes[i]);errno=0;feclearexcept(FE_ALL_EXCEPT);if(pow(3,2)!=9||pow(-3,3)!=-27||pow(1.5,4)!=5.0625||pow(16,0.25)!=2||pow(25,0.5)!=5||pow(2,-1074)!=0x1p-1074||powf(2,-149)!=0x1p-149f)return 1;if(errno||fetestexcept(FE_ALL_EXCEPT))return 2;}return 0;}') 0
    Check 'pow-integer-parity' ($prefix + 'int main(void){if(pow(-1,0x1p53)!=1||pow(-1,9007199254740991.0)!=-1||powf(-1,16777215.0f)!=-1||powf(-1,16777216.0f)!=1)return 1;return pow(-2,-3)!=-0.125;}') 0
    Check 'pow-tiny-exponent-direction' ($prefix + 'int main(void){union{double f;unsigned long long u;}y,v;y.u=1;fesetround(FE_UPWARD);v.f=pow(2,y.f);if(v.u!=0x3ff0000000000001ULL)return 1;fesetround(FE_DOWNWARD);v.f=pow(0.5,y.f);return v.u!=0x3fefffffffffffffULL;}') 0
    Check 'pow-fractional-power-of-two-direction' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;fesetround(FE_DOWNWARD);v.f=pow(8,0.3333333333333333);return v.u!=0x3fffffffffffffffULL;}') 0
    Check 'pow-range-directed-flags' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;fesetround(FE_UPWARD);errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=pow(2,-1075);if(v.u!=1||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_UNDERFLOW|FE_INEXACT))return 1;fesetround(FE_DOWNWARD);errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=pow(-2,1025);if(v.u!=0xfff0000000000000ULL||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_OVERFLOW|FE_INEXACT))return 2;fesetround(FE_UPWARD);v.f=pow(-2,1025);return v.u!=0xffefffffffffffffULL;}') 0
    Check 'pow-nan-and-environment' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,v;fenv_t e;x.u=0x7ff8000000000033ULL;if(pow(x.f,0)!=1||pow(1,x.f)!=1)return 1;v.f=pow(2,x.f);if(v.u!=x.u)return 2;x.u=0xfff0000000000033ULL;feclearexcept(FE_ALL_EXCEPT);v.f=pow(x.f,0);if(v.u!=0xfff8000000000033ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 3;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_UPWARD);feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);errno=73;v.f=pow(1.01,100.3);fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040||fegetround()!=FE_UPWARD||errno!=73||fetestexcept(FE_ALL_EXCEPT)!=(FE_DIVBYZERO|FE_INEXACT);}') 0
    Check 'pow-exact-fractional-roots' ($prefix + 'int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;for(i=0;i<4;i++){fesetround(modes[i]);feclearexcept(FE_ALL_EXCEPT);if(pow(9,1.5)!=27||pow(81,0.75)!=27||pow(2.25,1.5)!=3.375||powf(81,0.75f)!=27||powl(81,0.75L)!=27)return 1;if(fetestexcept(FE_ALL_EXCEPT))return 2;}return 0;}') 0
    $rng = [Random]::new(775931)
    foreach ($precision in 53, 24) {
        $rows = for ($i = 0; $i -lt 1024; $i++) {
            $x = [C1Math]::ScaleB(0.5 + $rng.NextDouble(), $rng.Next(-1000, 1000))
            $y = ($rng.NextDouble() - 0.5) * 8
            if ($i -band 1) {
                $x = 0.5 + $rng.NextDouble()
                $y = ($rng.NextDouble() - 0.5) * 2000
            }
            if ($precision -eq 24) {
                $x = [float]([C1Math]::ScaleB(0.5 + $rng.NextDouble(), $rng.Next(-120, 120)))
                $y = [float]$y
                $r = [C1MathF]::Pow($x, $y)
                $a = [C1Bits]::SingleToUInt32Bits($x)
                $b = [C1Bits]::SingleToUInt32Bits($y)
                $c = [C1Bits]::SingleToUInt32Bits($r)
                $suffix = 'U'
            }
            else {
                $r = [Math]::Pow($x, $y)
                $a = [C1Bits]::DoubleToUInt64Bits($x)
                $b = [C1Bits]::DoubleToUInt64Bits($y)
                $c = [C1Bits]::DoubleToUInt64Bits($r)
                $suffix = 'ULL'
            }
            '{0x' + $a.ToString('X') + $suffix + ',0x' + $b.ToString('X') + $suffix + ',0x' + $c.ToString('X') + $suffix + '}'
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
            'pow'
        }
        else {
            'powf'
        }
        Check "pow-independent-$precision" ($prefix + 'static ' + $integer + ' rows[][3]={' + ($rows -join ',') + '};int main(void){int i;' + $integer + ' d;union{' + $type + ' f;' + $integer + ' u;}x,y,r;for(i=0;i<1024;i++){x.u=rows[i][0];y.u=rows[i][1];r.f=' + $fn + '(x.f,y.f);d=r.u>rows[i][2]?r.u-rows[i][2]:rows[i][2]-r.u;if(d>1)return 1;}return 0;}') 0
    }
    . (Join-Path $PSScriptRoot 'high-precision-tests.ps1')
    . (Join-Path $PSScriptRoot 'integer-tests.ps1')
}
finally {
    $env:C1_INCLUDE = $old
}
