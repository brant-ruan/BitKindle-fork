
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'hyperbolic-zero-infinity' ($prefix + 'int main(void){union{double f;unsigned long long u;}x;x.f=sinh(-0.0);if(x.u!=0x8000000000000000ULL)return 1;x.f=tanh(-0.0);if(x.u!=0x8000000000000000ULL||cosh(-0.0)!=1)return 2;if(sinh(-HUGE_VAL)!=-HUGE_VAL||cosh(-HUGE_VAL)!=HUGE_VAL||tanh(-HUGE_VAL)!=-1||tanh(HUGE_VAL)!=1)return 3;return 0;}') 0
    Check 'hyperbolic-signaling-nan' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,y;x.u=0xfff0000000000023ULL;feclearexcept(FE_ALL_EXCEPT);y.f=cosh(x.f);return y.u!=0xfff8000000000023ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'hyperbolic-tiny-directed' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}x,y;x.f=0x1p-60;for(i=0;i<4;i++){fesetround(modes[i]);y.f=sinh(x.f);if(y.u!=x.u+(i==1))return 1;y.f=sinh(-x.f);if(y.u!=(x.u+(i==2))+(1ULL<<63))return 2;y.f=tanh(x.f);if(y.u!=x.u-(i==2||i==3))return 3;y.f=cosh(x.f);if(y.u!=0x3ff0000000000000ULL+(i==1))return 4;}return 0;}
'@) 0
    Check 'hyperbolic-saturation-directed' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}y;for(i=0;i<4;i++){fesetround(modes[i]);y.f=tanh(1000);if(y.u!=0x3ff0000000000000ULL-(i==2||i==3))return 1;y.f=tanh(-1000);if(y.u!=0xbff0000000000000ULL-(i==1||i==3))return 2;}return 0;}
'@) 0
    Check 'hyperbolic-overflow-directed' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}y;for(i=0;i<4;i++){fesetround(modes[i]);errno=0;feclearexcept(FE_ALL_EXCEPT);y.f=sinh(-1000);if(y.u!=(i==0||i==2?0xfff0000000000000ULL:0xffefffffffffffffULL)||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_OVERFLOW|FE_INEXACT))return 1;}return 0;}
'@) 0
    Check 'hyperbolic-subnormal-ftz-daz' ($prefix + 'int main(void){fenv_t e;union{double f;unsigned long long u;}x,y;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_UPWARD);x.u=1;errno=0;y.f=sinh(x.f);if(y.u!=2||errno!=ERANGE)return 1;fesetround(FE_TOWARDZERO);y.f=tanh(x.f);if(y.u)return 2;fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040;}') 0
    $rng = [Random]::new(51182)
    foreach ($fn in 'sinh', 'cosh', 'tanh') {
        $values = @(-0.5, 0.5, -0.49999999999999994, 0.49999999999999994, -0.1, 0.1, -1, 1, -20, 20, 39.99999999999999, 40.0, 710.0, 710.4758600739439)
        $range = if ($fn -eq 'tanh') {
            25.0
        }
        else {
            710.0
        }
        for ($i = 0; $i -lt 256; $i++) {
            $values += ($rng.NextDouble() * 2 - 1) * $range
        }
        $rows = foreach ($x in $values) {
            $y = switch ($fn) {
                'sinh' {
                    [Math]::Sinh($x)
                }'cosh' {
                    [Math]::Cosh($x)
                }'tanh' {
                    [Math]::Tanh($x)
                }
            }
            '{0x' + ([C1Bits]::DoubleToUInt64Bits($x)).ToString('X') + 'ULL,0x' + ([C1Bits]::DoubleToUInt64Bits($y)).ToString('X') + 'ULL}'
        }
        $oracle = $prefix + 'static unsigned long long rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned long long d;union{double f;unsigned long long u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){x.u=rows[i][0];y.f=' + $fn + '(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return i+1;y.f=(double)' + $fn + 'l((long double)x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return i+1;}return 0;}'
        Check ($fn + '-independent-270-double-long-double') $oracle 0
    }
    foreach ($fn in 'sinh', 'cosh', 'tanh') {
        $range = if ($fn -eq 'tanh') {
            25.0
        }
        else {
            89.0
        }
        $rows = for ($i = 0; $i -lt 256; $i++) {
            $x = [float](($rng.NextDouble() * 2 - 1) * $range)
            $y = switch ($fn) {
                'sinh' {
                    [C1MathF]::Sinh($x)
                }'cosh' {
                    [C1MathF]::Cosh($x)
                }'tanh' {
                    [C1MathF]::Tanh($x)
                }
            }
            '{0x' + ([C1Bits]::SingleToUInt32Bits($x)).ToString('X') + 'U,0x' + ([C1Bits]::SingleToUInt32Bits($y)).ToString('X') + 'U}'
        }
        $oracle = $prefix + 'static unsigned int rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned int d;union{float f;unsigned int u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){x.u=rows[i][0];y.f=' + $fn + 'f(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return i+1;}return 0;}'
        Check ($fn + '-independent-256-float') $oracle 0
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
