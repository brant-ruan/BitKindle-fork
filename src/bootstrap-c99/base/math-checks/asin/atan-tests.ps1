
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'atan-signed-zero' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;feclearexcept(FE_ALL_EXCEPT);v.f=atan(-0.0);return v.u!=0x8000000000000000ULL||atan(0)!=0||fetestexcept(FE_ALL_EXCEPT);}') 0
    Check 'atan-nan' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;v.u=0xfff0000000000042ULL;feclearexcept(FE_ALL_EXCEPT);v.f=atan(v.f);return v.u!=0xfff8000000000042ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'atan-infinite-directed' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}v;for(i=0;i<4;i++){fesetround(modes[i]);v.f=atan(HUGE_VAL);if(v.u!=0x3ff921fb54442d18ULL+(i==1))return 1;v.f=atan(-HUGE_VAL);if(v.u!=0xbff921fb54442d18ULL+(i==2))return 2;if(fegetround()!=modes[i])return 3;}return 0;}
'@) 0
    Check 'atan-tiny-directed' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}v,x;x.f=0x1p-60;for(i=0;i<4;i++){fesetround(modes[i]);v.f=atan(x.f);if(v.u!=x.u-(i==2||i==3))return 1;v.f=atan(-x.f);if(v.u!=((x.u-(i==1||i==3))|0x8000000000000000ULL))return 2;}return 0;}
'@) 0
    Check 'atan-subnormal-ftz-daz' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;fenv_t e;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_TOWARDZERO);v.u=1;errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=atan(v.f);if(v.u||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_UNDERFLOW|FE_INEXACT))return 1;fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040;}') 0
    $rng = [Random]::new(84761)
    $rows = for ($i = 0; $i -lt 512; $i++) {
        $bits = ([uint64]$rng.Next(0, 2047) -shl 52) -bor ([uint64]$rng.Next(0, 1048576) -shl 32) -bor [uint64]$rng.Next(1, 2147483647)
        if ($i -band 1) {
            $bits = $bits -bor ([Convert]::ToUInt64('8000000000000000', 16))
        }
        $x = [C1Bits]::UInt64BitsToDouble($bits)
        $y = [Math]::Atan($x)
        '{0x' + $bits.ToString('X') + 'ULL,0x' + ([C1Bits]::DoubleToUInt64Bits($y)).ToString('X') + 'ULL}'
    }
    $oracle = $prefix + 'static unsigned long long rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned long long d;union{double f;unsigned long long u;}x,y;for(i=0;i<512;i++){x.u=rows[i][0];y.f=atan(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 1;y.f=(double)atanl((long double)x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 2;}return 0;}'
    Check 'atan-independent-512-double-long-double' $oracle 0
    $rows = for ($i = 0; $i -lt 512; $i++) {
        $bits = ([uint32]$rng.Next(0, 255) -shl 23) -bor [uint32]$rng.Next(1, 8388608)
        if ($i -band 1) {
            $bits = $bits -bor ([Convert]::ToUInt32('80000000', 16))
        }
        $x = [C1Bits]::UInt32BitsToSingle($bits)
        $y = [C1MathF]::Atan($x)
        '{0x' + $bits.ToString('X') + 'U,0x' + ([C1Bits]::SingleToUInt32Bits($y)).ToString('X') + 'U}'
    }
    $oracle = $prefix + 'static unsigned int rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned int d;union{float f;unsigned int u;}x,y;for(i=0;i<512;i++){x.u=rows[i][0];y.f=atanf(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 1;}return 0;}'
    Check 'atan-independent-512-float' $oracle 0
    . (Join-Path $PSScriptRoot 'atan-high-precision-tests.ps1')
}
finally {
    if ($null -eq $savedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $savedInclude
    }
}
