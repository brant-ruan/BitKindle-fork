
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$atan2SavedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'atan2-signed-zero-quadrants' ($prefix + @'
int main(void){union{double f;unsigned long long u;}v;v.f=atan2(0.0,0.0);if(v.u)return 1;v.f=atan2(-0.0,0.0);if(v.u!=0x8000000000000000ULL)return 2;v.f=atan2(0.0,-0.0);if(v.u!=0x400921fb54442d18ULL)return 3;v.f=atan2(-0.0,-0.0);if(v.u!=0xc00921fb54442d18ULL)return 4;v.f=atan2(-1,0);return v.u!=0xbff921fb54442d18ULL;}
'@) 0
    Check 'atan2-infinities' ($prefix + @'
int main(void){union{double f;unsigned long long u;}v;v.f=atan2(HUGE_VAL,HUGE_VAL);if(v.u!=0x3fe921fb54442d18ULL)return 1;v.f=atan2(HUGE_VAL,-HUGE_VAL);if(v.u!=0x4002d97c7f3321d2ULL)return 2;v.f=atan2(-HUGE_VAL,-HUGE_VAL);if(v.u!=0xc002d97c7f3321d2ULL)return 3;v.f=atan2(-1,HUGE_VAL);if(v.u!=0x8000000000000000ULL)return 4;return 0;}
'@) 0
    Check 'atan2-nan-priority-and-signaling' ($prefix + 'int main(void){union{double f;unsigned long long u;}a,b,v;a.u=0x7ff8000000000011ULL;b.u=0xfff0000000000022ULL;feclearexcept(FE_ALL_EXCEPT);v.f=atan2(a.f,b.f);return v.u!=a.u||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'atan2-half-subnormal-double' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}y,v;y.u=3;for(i=0;i<4;i++){fesetround(modes[i]);errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=atan2(y.f,2);if(v.u!=(i==1?2ULL:1ULL)||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_UNDERFLOW|FE_INEXACT))return 1;}return 0;}
'@) 0
    Check 'atan2-half-subnormal-float' ($prefix + 'int main(void){union{float f;unsigned int u;}y,v;y.u=3;v.f=atan2f(y.f,2);if(v.u!=1)return 1;fesetround(FE_UPWARD);v.f=atan2f(y.f,2);return v.u!=2;}') 0
    Check 'atan2-extreme-underflow' ($prefix + 'int main(void){union{double f;unsigned long long u;}y,x,v;y.u=1;x.u=0x7fefffffffffffffULL;fesetround(FE_UPWARD);v.f=atan2(y.f,x.f);if(v.u!=1)return 1;fesetround(FE_DOWNWARD);y.u|=0x8000000000000000ULL;v.f=atan2(y.f,x.f);return v.u!=0x8000000000000001ULL;}') 0
    Check 'atan2-ftz-daz-and-ratio-direction' ($prefix + 'int main(void){fenv_t e;union{double f;unsigned long long u;}y,v;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_DOWNWARD);y.f=0x1p-100;v.f=atan2(y.f,1);if(v.u!=y.u-1)return 1;fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040||fegetround()!=FE_DOWNWARD;}') 0
    $rng = [Random]::new(29471)
    $rows = for ($i = 0; $i -lt 512; $i++) {
        $a = ([uint64]$rng.Next(0, 2047) -shl 52) -bor ([uint64]$rng.Next(0, 1048576) -shl 32) -bor [uint64]$rng.Next(1, 2147483647)
        $b = ([uint64]$rng.Next(0, 2047) -shl 52) -bor ([uint64]$rng.Next(0, 1048576) -shl 32) -bor [uint64]$rng.Next(1, 2147483647)
        if ($i -band 1) {
            $a = $a -bor ([Convert]::ToUInt64('8000000000000000', 16))
        }
        if ($i -band 2) {
            $b = $b -bor ([Convert]::ToUInt64('8000000000000000', 16))
        }
        $r = [Math]::Atan2([C1Bits]::UInt64BitsToDouble($a), [C1Bits]::UInt64BitsToDouble($b))
        '{0x' + $a.ToString('X') + 'ULL,0x' + $b.ToString('X') + 'ULL,0x' + ([C1Bits]::DoubleToUInt64Bits($r)).ToString('X') + 'ULL}'
    }
    $oracle = $prefix + 'static unsigned long long rows[][3]={' + ($rows -join ',') + '};int main(void){int i;unsigned long long d;union{double f;unsigned long long u;}y,x,r;for(i=0;i<512;i++){y.u=rows[i][0];x.u=rows[i][1];r.f=atan2(y.f,x.f);d=r.u>rows[i][2]?r.u-rows[i][2]:rows[i][2]-r.u;if(d>1)return 1;r.f=(double)atan2l((long double)y.f,(long double)x.f);d=r.u>rows[i][2]?r.u-rows[i][2]:rows[i][2]-r.u;if(d>1)return 2;}return 0;}'
    Check 'atan2-independent-512-double-long-double' $oracle 0
    $rows = for ($i = 0; $i -lt 512; $i++) {
        $a = ([uint32]$rng.Next(0, 255) -shl 23) -bor [uint32]$rng.Next(1, 8388608)
        $b = ([uint32]$rng.Next(0, 255) -shl 23) -bor [uint32]$rng.Next(1, 8388608)
        if ($i -band 1) {
            $a = $a -bor ([Convert]::ToUInt32('80000000', 16))
        }
        if ($i -band 2) {
            $b = $b -bor ([Convert]::ToUInt32('80000000', 16))
        }
        $r = [C1MathF]::Atan2([C1Bits]::UInt32BitsToSingle($a), [C1Bits]::UInt32BitsToSingle($b))
        '{0x' + $a.ToString('X') + 'U,0x' + $b.ToString('X') + 'U,0x' + ([C1Bits]::SingleToUInt32Bits($r)).ToString('X') + 'U}'
    }
    $oracle = $prefix + 'static unsigned int rows[][3]={' + ($rows -join ',') + '};int main(void){int i;unsigned int d;union{float f;unsigned int u;}y,x,r;for(i=0;i<512;i++){y.u=rows[i][0];x.u=rows[i][1];r.f=atan2f(y.f,x.f);d=r.u>rows[i][2]?r.u-rows[i][2]:rows[i][2]-r.u;if(d>1)return 1;}return 0;}'
    Check 'atan2-independent-512-float' $oracle 0
    . (Join-Path $PSScriptRoot 'high-precision-tests.ps1')
}
finally {
    if ($null -eq $atan2SavedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $atan2SavedInclude
    }
}
