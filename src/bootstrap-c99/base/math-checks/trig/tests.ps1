
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$old = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'trig-zero-domain' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;v.f=sin(-0.0);if(v.u!=0x8000000000000000ULL)return 1;v.f=tan(-0.0);if(v.u!=0x8000000000000000ULL||cos(-0.0)!=1)return 2;errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=sin(HUGE_VAL);return !isnan(v.f)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID;}') 0
    Check 'trig-tiny-rounding' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,v;x.u=1;fesetround(FE_DOWNWARD);v.f=sin(x.f);if(v.u)return 1;v.f=cos(x.f);if(v.u!=0x3fefffffffffffffULL)return 2;fesetround(FE_UPWARD);v.f=tan(x.f);return v.u!=2;}') 0
    $rng = [Random]::new(15934)
    foreach ($fun in @('sin', 'cos', 'tan')) {
        $rows = for ($i = 0; $i -lt 1024; $i++) {
            $a = ([uint64]$rng.Next(0, 2047) -shl 52) -bor ([uint64]$rng.Next(0, 1048576) -shl 32) -bor [uint64]$rng.Next(1, 2147483647)
            if ($i -band 1) {
                $a = $a -bor ([Convert]::ToUInt64('8000000000000000', 16))
            }
            if ($i -eq 0) {
                $a = ([Convert]::ToUInt64('7fefffffffffffff', 16))
            }
            $x = [C1Bits]::UInt64BitsToDouble($a)
            $r = switch ($fun) {
                sin {
                    [C1Math]::Sin($x)
                }cos {
                    [C1Math]::Cos($x)
                }tan {
                    [C1Math]::Tan($x)
                }
            }
            '{0x' + $a.ToString('X') + 'ULL,0x' + ([C1Bits]::DoubleToUInt64Bits($r)).ToString('X') + 'ULL}'
        }
        Check "trig-$fun-full-exponent-double" ($prefix + 'static unsigned long long rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned long long d;union{double f;unsigned long long u;}x,r;for(i=0;i<1024;i++){x.u=rows[i][0];r.f=' + $fun + '(x.f);d=r.u>rows[i][1]?r.u-rows[i][1]:rows[i][1]-r.u;if(d>1)return 1;r.f=(double)' + $fun + 'l((long double)x.f);d=r.u>rows[i][1]?r.u-rows[i][1]:rows[i][1]-r.u;if(d>1)return 2;}return 0;}') 0
    }
    . (Join-Path $PSScriptRoot 'high-precision-tests.ps1')
    . (Join-Path $PSScriptRoot 'edge-tests.ps1')
}
finally {
    $env:C1_INCLUDE = $old
}
