
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'log-domain-and-poles' ($prefix + @'
int main(void){double (*f[3])(double)={log,log10,log2};int i;union{double f;unsigned long long u;}y;for(i=0;i<3;i++){errno=0;feclearexcept(FE_ALL_EXCEPT);y.f=f[i](-1);if(!isnan(y.f)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 1;errno=0;feclearexcept(FE_ALL_EXCEPT);y.f=f[i](-0.0);if(y.u!=0xfff0000000000000ULL||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=FE_DIVBYZERO)return 2;}return 0;}
'@) 0
    Check 'log-exact-one-and-infinity' ($prefix + 'int main(void){feclearexcept(FE_ALL_EXCEPT);errno=EDOM;if(log(1)||log10(1)||log2(1)||logf(1)||logl(1))return 1;if(log(HUGE_VAL)!=HUGE_VAL||log10(HUGE_VAL)!=HUGE_VAL||log2(HUGE_VAL)!=HUGE_VAL)return 2;return fetestexcept(FE_ALL_EXCEPT)||errno!=EDOM;}') 0
    Check 'log-signaling-nan' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,y;x.u=0xfff0000000001234ULL;feclearexcept(FE_ALL_EXCEPT);y.f=log(x.f);return y.u!=0xfff8000000001234ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'log2-exact-all-double-powers' ($prefix + 'int main(void){int i;double x,y;for(i=-1074;i<=1023;i++){x=ldexp(1,i);feclearexcept(FE_ALL_EXCEPT);y=log2(x);if(y!=i||fetestexcept(FE_ALL_EXCEPT))return 1;}return 0;}') 0
    Check 'log2-exact-all-float-powers' ($prefix + 'int main(void){int i;float x,y;for(i=-149;i<=127;i++){x=ldexpf(1,i);feclearexcept(FE_ALL_EXCEPT);y=log2f(x);if(y!=i||fetestexcept(FE_ALL_EXCEPT))return 1;}return 0;}') 0
    Check 'log10-exact-decimal-powers' ($prefix + 'int main(void){double x=1;int i;for(i=0;i<=22;i++){feclearexcept(FE_ALL_EXCEPT);if(log10(x)!=i||fetestexcept(FE_ALL_EXCEPT))return 1;x*=10;}return 0;}') 0
    Check 'log-ftz-daz-environment' ($prefix + 'int main(void){fenv_t e;union{double f;unsigned long long u;}x,y;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_UPWARD);feraiseexcept(FE_DIVBYZERO);x.u=1;y.f=log(x.f);if(y.f> -744||y.f< -745||fegetround()!=FE_UPWARD)return 1;fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040||fetestexcept(FE_ALL_EXCEPT)!=(FE_DIVBYZERO|FE_INEXACT);}') 0
    $rng = [Random]::new(92180)
    foreach ($base in @('log', 'log10', 'log2')) {
        $rows = for ($i = 0; $i -lt 512; $i++) {
            $field = $rng.Next(1, 2047)
            $fraction = ([uint64]$rng.Next(0, 1048576) -shl 32) -bor [uint64]$rng.Next(0, 2147483647)
            $bits = ([uint64]$field -shl 52) -bor $fraction
            if ($i -eq 0) {
                $bits = ([uint64]1)
            }
            if ($i -eq 1) {
                $bits = ([Convert]::ToUInt64('3fefffffffffffff', 16))
            }
            if ($i -eq 2) {
                $bits = ([Convert]::ToUInt64('3ff0000000000001', 16))
            }
            $x = [C1Bits]::UInt64BitsToDouble($bits)
            $y = switch ($base) {
                'log' {
                    [Math]::Log($x)
                }'log10' {
                    [Math]::Log10($x)
                }'log2' {
                    [C1Math]::Log2($x)
                }
            }
            '{0x' + $bits.ToString('X') + 'ULL,0x' + ([C1Bits]::DoubleToUInt64Bits($y)).ToString('X') + 'ULL}'
        }
        $oracle = $prefix + 'static unsigned long long rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned long long d;union{double f;unsigned long long u;}x,y;for(i=0;i<512;i++){x.u=rows[i][0];y.f=' + $base + '(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 1;y.f=(double)' + $base + 'l((long double)x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 2;}return 0;}'
        Check ($base + '-independent-512-double-long-double') $oracle 0
    }
    foreach ($base in @('log', 'log10', 'log2')) {
        $rows = for ($i = 0; $i -lt 512; $i++) {
            $bits = ([uint32]$rng.Next(0, 255) -shl 23) -bor [uint32]$rng.Next(1, 8388608)
            $x = [C1Bits]::UInt32BitsToSingle($bits)
            $y = switch ($base) {
                'log' {
                    [C1MathF]::Log($x)
                }'log10' {
                    [C1MathF]::Log10($x)
                }'log2' {
                    [C1MathF]::Log2($x)
                }
            }
            '{0x' + $bits.ToString('X') + 'U,0x' + ([C1Bits]::SingleToUInt32Bits($y)).ToString('X') + 'U}'
        }
        $oracle = $prefix + 'static unsigned int rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned int d;union{float f;unsigned int u;}x,y;for(i=0;i<512;i++){x.u=rows[i][0];y.f=' + $base + 'f(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return 1;}return 0;}'
        Check ($base + '-independent-512-float') $oracle 0
    }
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
