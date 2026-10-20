
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$old = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include')) + ';' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../include'))
    $prefix = '#include "' + [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../lib/runtime.c')).Replace('\', '/') + '"' + [char]10
    Check 'log1p-domain-pole-special' ($prefix + 'int main(void){union{double f;unsigned long long u;}v;errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=log1p(-1);if(v.u!=0xfff0000000000000ULL||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=FE_DIVBYZERO)return 1;errno=0;feclearexcept(FE_ALL_EXCEPT);v.f=log1p(-1.01);if(!isnan(v.f)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 2;v.f=log1p(-0.0);if(v.u!=0x8000000000000000ULL||log1p(HUGE_VAL)!=HUGE_VAL)return 3;v.u=0xfff0000000000044ULL;v.f=log1p(v.f);return v.u!=0xfff8000000000044ULL;}') 0
    Check 'log1p-subnormal-direction' ($prefix + 'int main(void){union{double f;unsigned long long u;}x,r;x.u=1;fesetround(FE_DOWNWARD);errno=0;feclearexcept(FE_ALL_EXCEPT);r.f=log1p(x.f);if(r.u||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_INEXACT|FE_UNDERFLOW))return 1;x.u=0x8000000000000001ULL;r.f=log1p(x.f);return r.u!=0x8000000000000002ULL;}') 0
    Check 'log1p-environment' ($prefix + 'int main(void){fenv_t e;double x;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_UPWARD);feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);errno=73;x=log1p(0.1);fegetenv(&e);return x<=0||(e.__mxcsr&0x8040)!=0x8040||fegetround()!=FE_UPWARD||errno!=73||fetestexcept(FE_ALL_EXCEPT)!=(FE_INEXACT|FE_DIVBYZERO);}') 0
    . (Join-Path $PSScriptRoot 'oracle-helpers.ps1')
    function ReferenceLog1p([double]$x, [int]$precision, [int]$mode) {
        $raw = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
        $field = [int](($raw -shr 52) -band 2047)
        $m = [bigint]($raw -band ([Convert]::ToUInt64('fffffffffffff', 16)))
        if ($field) {
            $m += [bigint]::One -shl 52
        }
        else {
            $field = 1
        }
        $arg = $m -shl ($field - 1023 - 52 + $script:logOracleFraction)
        if ($x -lt 0) {
            $arg = - $arg
        }
        $arg += $script:logOracleUnit
        $e = [int][C1Bits]::GetBitLength($arg) - 1 - $script:logOracleFraction
        $sum = (FixedNaturalLog ($arg -shr $e)) + $e * $script:logOracleLn2
        $negative = $sum.Sign -lt 0
        if ($negative) {
            $sum = - $sum
            if ($mode -eq 1) {
                $mode = 2
            }
            elseif ($mode -eq 2) {
                $mode = 1
            }
        }
        $p = $precision - 1
        $bias = if ($precision -eq 24) {
            127
        }
        else {
            1023
        }
        $e = [int][C1Bits]::GetBitLength($sum) - 1 - $script:logOracleFraction
        $field = $e + $bias
        $shift = $script:logOracleFraction + [Math]::Max($e, 1 - $bias) - $p
        $out = $sum -shr $shift
        $rest = $sum - ($out -shl $shift)
        if ($mode -eq 1 -and !$rest.IsZero) {
            $out += [bigint]::One
        }
        if ($mode -eq 0) {
            $half = [bigint]::One -shl ($shift - 1)
            if ($rest -gt $half -or ($rest -eq $half -and !$out.IsEven)) {
                $out += [bigint]::One
            }
        }
        if ($field -gt 0) {
            $out += ([bigint]($field - 1)) -shl $p
        }
        $bits = [uint64]$out
        if ($negative) {
            $bits = $bits -bor $(if ($precision -eq 24) {
                    ([Convert]::ToUInt64('80000000', 16))
                }
                else {
                    ([Convert]::ToUInt64('8000000000000000', 16))
                })
        }
        return $bits
    }
    foreach ($precision in 53, 24) {
        $inputs = @(-0.9, -0.5, -0.1, -1e-20, 1e-20, 0.1, 0.5, 1, 3, 100)
        if ($precision -eq 53) {
            $inputs += @([C1Math]::BitIncrement(-1), 1e300, [double]::MaxValue)
            $tiny = [C1Math]::ScaleB(1, -54)
        }
        else {
            $inputs += @([double][C1MathF]::BitIncrement(-1), 1e30, [double][float]::MaxValue)
            $tiny = [C1Math]::ScaleB(1, -25)
        }
        foreach ($center in @(-0.5, 0.5, $tiny, - $tiny)) {
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
                $expected = ReferenceLog1p $x $precision $mode
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
            'log1p'
        }
        else {
            'log1pf'
        }
        $extra = if ($precision -eq 53) {
            'r.f=(double)log1pl((long double)x.f);if(r.u!=rows[i][1])return 2;'
        }
        else {
            ''
        }
        Check "log1p-high-precision-$precision" ($prefix + 'static ' + $integer + ' rows[][3]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,r;for(i=0;i<' + $rows.Count + ';i++){x.u=rows[i][0];fesetround(modes[rows[i][2]]);r.f=' + $fn + '(x.f);if(r.u!=rows[i][1])return 1;' + $extra + '}return 0;}') 0
    }
    . (Join-Path $PSScriptRoot 'random-tests.ps1')
}
finally {
    $env:C1_INCLUDE = $old
}
