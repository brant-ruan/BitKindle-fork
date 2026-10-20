
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'exp-zero-and-infinities' ($prefix + 'int main(void){if(exp(0)!=1||exp(-0.0)!=1||expf(0)!=1||expl(0)!=1)return 1;if(exp(-HUGE_VAL)!=0||exp(HUGE_VAL)!=HUGE_VAL)return 2;return 0;}') 0
    Check 'exp-nan-payload' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,y;x.u=0xfff0000000000123ULL;feclearexcept(FE_ALL_EXCEPT);y.f=exp(x.f);if(y.u!=0xfff8000000000123ULL||!fetestexcept(FE_INVALID))return 1;x.u=0x7ff8000000000456ULL;feclearexcept(FE_ALL_EXCEPT);y.f=exp(x.f);return y.u!=x.u||fetestexcept(FE_ALL_EXCEPT);}') 0
    Check 'exp-round-tiny' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}y;for(i=0;i<4;i++){fesetround(modes[i]);y.f=exp(0x1p-60);if(y.u!=(i==1?0x3ff0000000000001ULL:0x3ff0000000000000ULL))return 1;y.f=exp(-0x1p-60);if(y.u!=(i==2||i==3?0x3fefffffffffffffULL:0x3ff0000000000000ULL))return 2;if(fegetround()!=modes[i])return 3;}return 0;}
'@) 0
    Check 'exp-overflow-modes' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}y;for(i=0;i<4;i++){fesetround(modes[i]);feclearexcept(FE_ALL_EXCEPT);errno=0;y.f=exp(1000);if(y.u!=(i<2?0x7ff0000000000000ULL:0x7fefffffffffffffULL)||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_OVERFLOW|FE_INEXACT))return 1;}return 0;}
'@) 0
    Check 'exp-underflow-modes' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}y;for(i=0;i<4;i++){fesetround(modes[i]);feclearexcept(FE_ALL_EXCEPT);errno=0;y.f=exp(-1000);if(y.u!=(i==1?1ULL:0ULL)||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_UNDERFLOW|FE_INEXACT))return 1;}return 0;}
'@) 0
    Check 'exp-preserve-environment' ($prefix + 'int main(void){fenv_t before,after;fesetround(FE_UPWARD);feraiseexcept(FE_DIVBYZERO);errno=EDOM;fegetenv(&before);exp(1);fegetenv(&after);return fegetround()!=FE_UPWARD||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=(FE_DIVBYZERO|FE_INEXACT)||(before.__mxcsr&~63U)!=(after.__mxcsr&~63U);}') 0
    Check 'exp-float-round-tiny' ($prefix + 'int main(void){union{float f;unsigned int u;}y;fesetround(FE_UPWARD);y.f=expf(0x1p-30f);if(y.u!=0x3f800001U)return 1;fesetround(FE_DOWNWARD);y.f=expf(-0x1p-30f);return y.u!=0x3f7fffffU;}') 0
    Check 'exp-ftz-daz-and-subnormal-input' ($prefix + @'
int main(void){fenv_t e;union{double f;unsigned long long u;}x,y,z;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_UPWARD);x.u=1;y.f=exp(x.f);if(y.u!=0x3ff0000000000001ULL)return 1;fesetround(FE_DOWNWARD);x.u=0x8000000000000001ULL;y.f=exp(x.f);if(y.u!=0x3fefffffffffffffULL)return 2;fesetround(FE_TONEAREST);y.f=exp(-744);z.f=expl(-744.L);if(!y.u||y.u!=z.u)return 3;fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040;}
'@) 0
    $rng = [Random]::new(38114)
    $values = @(-745.2, -745.1332191019411, -745, -744, -710, -709, -708.3964185322641, -1, -0.1, 0, 0.1, 1, 10, 709, 709.782712893384, 709.7827128933841)
    for ($i = 0; $i -lt 512; $i++) {
        $values += -744.0 + $rng.NextDouble() * 1453.0
    }
    $rows = foreach ($v in $values) {
        $x = [C1Bits]::DoubleToUInt64Bits($v)
        $y = [C1Bits]::DoubleToUInt64Bits([Math]::Exp($v))
        '{0x' + $x.ToString('X') + 'ULL,0x' + $y.ToString('X') + 'ULL}'
    }
    $oracle = $prefix + 'static unsigned long long rows[][2]={' + ($rows -join ',') + '};int main(void){int i;union{double f;unsigned long long u;}x,y;unsigned long long d;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){x.u=rows[i][0];y.f=exp(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 1;}return 0;}'
    Check 'exp-double-independent-528' $oracle 0
    $rows = for ($i = 0; $i -lt 512; $i++) {
        $v = [float](-103.0 + $rng.NextDouble() * 191.7)
        $x = [C1Bits]::SingleToUInt32Bits($v)
        $y = [C1Bits]::SingleToUInt32Bits([C1MathF]::Exp($v))
        '{0x' + $x.ToString('X') + 'U,0x' + $y.ToString('X') + 'U}'
    }
    $oracle = $prefix + 'static unsigned int rows[][2]={' + ($rows -join ',') + '};int main(void){int i;union{float f;unsigned int u;}x,y;unsigned int d;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){x.u=rows[i][0];y.f=expf(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 1;}return 0;}'
    Check 'exp-float-independent-512' $oracle 0
    . (Join-Path $PSScriptRoot 'high-precision-tests.ps1')
}
finally {
    if ($null -eq $savedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $savedInclude
    }
}
