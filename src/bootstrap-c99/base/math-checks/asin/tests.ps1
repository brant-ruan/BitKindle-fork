
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$asinSavedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'inverse-domain' ($prefix + 'int main(void){double (*f[2])(double)={asin,acos};int i;for(i=0;i<2;i++){errno=0;feclearexcept(FE_ALL_EXCEPT);if(!isnan(f[i](1.0000000000000002))||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 1;errno=0;if(!isnan(f[i](-HUGE_VAL))||errno!=EDOM)return 2;}return 0;}') 0
    Check 'inverse-exact-zeros' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;feclearexcept(FE_ALL_EXCEPT);v.f=asin(-0.0);if(v.u!=0x8000000000000000ULL)return 1;v.f=acos(1);return v.u||fetestexcept(FE_ALL_EXCEPT);}') 0
    Check 'inverse-endpoints-directed' ($prefix + @'
int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}v;for(i=0;i<4;i++){fesetround(modes[i]);v.f=asin(1);if(v.u!=0x3ff921fb54442d18ULL+(i==1))return 1;v.f=asin(-1);if(v.u!=0xbff921fb54442d18ULL+(i==2))return 2;v.f=acos(-1);if(v.u!=0x400921fb54442d18ULL+(i==1))return 3;}return 0;}
'@) 0
    Check 'inverse-nan-payload' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,y;x.u=0x7ff0000000000034ULL;feclearexcept(FE_ALL_EXCEPT);y.f=asin(x.f);if(y.u!=0x7ff8000000000034ULL||!fetestexcept(FE_INVALID))return 1;y.f=acos(x.f);return y.u!=0x7ff8000000000034ULL;}') 0
    Check 'inverse-tiny-ftz-daz' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,y;fenv_t e;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_UPWARD);x.u=1;errno=0;y.f=asin(x.f);if(y.u!=2||errno!=ERANGE)return 1;y.f=acos(x.f);if(y.u!=0x3ff921fb54442d19ULL)return 2;fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040;}') 0
    $rng = [Random]::new(42791)
    foreach ($fn in 'asin', 'acos') {
        $values = @(-1.0, -0.9999999999999999, -0.5, -0.1, 0.0, 0.1, 0.5, 0.9999999999999999, 1.0)
        for ($i = 0; $i -lt 512; $i++) {
            $values += $rng.NextDouble() * 2 - 1
        }
        $rows = foreach ($x in $values) {
            $y = if ($fn -eq 'asin') {
                [Math]::Asin($x)
            }
            else {
                [Math]::Acos($x)
            }
            '{0x' + ([C1Bits]::DoubleToUInt64Bits($x)).ToString('X') + 'ULL,0x' + ([C1Bits]::DoubleToUInt64Bits($y)).ToString('X') + 'ULL}'
        }
        $oracle = $prefix + 'static unsigned long long rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned long long d;union{double f;unsigned long long u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){x.u=rows[i][0];y.f=' + $fn + '(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return i+1;y.f=(double)' + $fn + 'l((long double)x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return i+1;}return 0;}'
        Check ($fn + '-independent-521-double-long-double') $oracle 0
        $rows = for ($i = 0; $i -lt 512; $i++) {
            $x = [float]($rng.NextDouble() * 2 - 1)
            $y = if ($fn -eq 'asin') {
                [C1MathF]::Asin($x)
            }
            else {
                [C1MathF]::Acos($x)
            }
            '{0x' + ([C1Bits]::SingleToUInt32Bits($x)).ToString('X') + 'U,0x' + ([C1Bits]::SingleToUInt32Bits($y)).ToString('X') + 'U}'
        }
        $oracle = $prefix + 'static unsigned int rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned int d;union{float f;unsigned int u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){x.u=rows[i][0];y.f=' + $fn + 'f(x.f);d=y.u>rows[i][1]?y.u-rows[i][1]:rows[i][1]-y.u;if(d>1)return i+1;}return 0;}'
        Check ($fn + '-independent-512-float') $oracle 0
    }
    . (Join-Path $PSScriptRoot 'high-precision-tests.ps1')
}
finally {
    if ($null -eq $asinSavedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $asinSavedInclude
    }
}
