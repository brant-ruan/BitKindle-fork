
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Oracle only. Normalize to [1,2), then sum the atanh series using 1400
# fractional bits. This does not use the implementation's floating constants.
$script:logOracleFraction = 1400
$script:logOracleUnit = [bigint]::One -shl $script:logOracleFraction
function FixedNaturalLog([bigint]$normalized) {
    $unit = $script:logOracleUnit
    $z = [bigint]::Divide(($normalized - $unit) * $unit, $normalized + $unit)
    $square = [bigint]::Divide($z * $z, $unit)
    $term = $z
    $sum = $z
    for ($n = 3; $n -lt 5000; $n += 2) {
        $term = [bigint]::Divide($term * $square, $unit)
        if ($term.IsZero) {
            break
        }
        $sum += [bigint]::Divide($term, $n)
    }
    return $sum * 2
}
$script:logOracleLn2 = FixedNaturalLog ($script:logOracleUnit * 2)
$script:logOracleLn10 = (FixedNaturalLog ([bigint]::Divide($script:logOracleUnit * 5, 4))) + 3 * $script:logOracleLn2
function ReferenceLogBits([double]$x, [int]$precision, [int]$base, [int]$mode) {
    $bits = [C1Bits]::DoubleToUInt64Bits($x)
    $field = [int](($bits -shr 52) -band 2047)
    $mantissa = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $mantissa += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $top = [int][C1Bits]::GetBitLength($mantissa) - 1
    $exponent = $field - 1023 - 52 + $top
    $normalized = $mantissa -shl ($script:logOracleFraction - $top)
    $y = (FixedNaturalLog $normalized) + $exponent * $script:logOracleLn2
    if ($base -eq 2) {
        $y = [bigint]::Divide($y * $script:logOracleUnit, $script:logOracleLn2)
    }
    if ($base -eq 10) {
        $y = [bigint]::Divide($y * $script:logOracleUnit, $script:logOracleLn10)
    }
    if ($y.IsZero) {
        return ([uint64]0)
    }
    $negative = $y.Sign -lt 0
    if ($negative) {
        $y = - $y
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
    $exponent = [int][C1Bits]::GetBitLength($y) - 1 - $script:logOracleFraction
    $shift = $script:logOracleFraction + $exponent - $p
    $q = $y -shr $shift
    $rest = $y - ($q -shl $shift)
    if ($mode -eq 1 -and !$rest.IsZero) {
        $q += [bigint]::One
    }
    if ($mode -eq 0) {
        $half = [bigint]::One -shl ($shift - 1)
        if ($rest -gt $half -or ($rest -eq $half -and !$q.IsEven)) {
            $q += [bigint]::One
        }
    }
    $q += ([bigint]($exponent + $bias - 1)) -shl $p
    $result = [uint64]$q
    if ($negative) {
        $result = $result -bor $(if ($precision -eq 24) {
                ([Convert]::ToUInt64('80000000', 16))
            }
            else {
                ([Convert]::ToUInt64('8000000000000000', 16))
            })
    }
    return $result
}
foreach ($base in 0, 10, 2) {
    $function = if ($base -eq 0) {
        'log'
    }
    else {
        'log' + $base
    }
    $inputs = @(([uint64]1), ([uint64]2), ([Convert]::ToUInt64('000fffffffffffff', 16)), ([Convert]::ToUInt64('0010000000000000', 16)), ([Convert]::ToUInt64('3fb999999999999a', 16)), ([Convert]::ToUInt64('3fefffffffffffff', 16)), ([Convert]::ToUInt64('3ff0000000000001', 16)), ([Convert]::ToUInt64('3ff4000000000000', 16)), ([Convert]::ToUInt64('4000000000000000', 16)), ([Convert]::ToUInt64('4024000000000000', 16)), ([Convert]::ToUInt64('7fefffffffffffff', 16)))
    $rows = foreach ($bits in $inputs) {
        $x = [C1Bits]::UInt64BitsToDouble($bits)
        foreach ($mode in 0..3) {
            $reference = ReferenceLogBits $x 53 $base $mode
            '{0x' + $bits.ToString('X') + 'ULL,0x' + $reference.ToString('X') + 'ULL,' + $mode + '}'
        }
    }
    $oracle = $prefix + 'static struct R{unsigned long long x,y;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].x;y.f=' + $function + '(x.f);if(y.u!=rows[i].y)return i+1;}return 0;}'
    Check ($function + '-fixed-point-44-double-directed') $oracle 0
    $inputs = @(([uint32]1), ([uint32]2), ([Convert]::ToUInt32('007fffff', 16)), ([Convert]::ToUInt32('00800000', 16)), ([Convert]::ToUInt32('3dcccccd', 16)), ([Convert]::ToUInt32('3f7fffff', 16)), ([Convert]::ToUInt32('3f800001', 16)), ([Convert]::ToUInt32('3fa00000', 16)), ([Convert]::ToUInt32('40000000', 16)), ([Convert]::ToUInt32('41200000', 16)), ([Convert]::ToUInt32('7f7fffff', 16)))
    $rows = foreach ($bits in $inputs) {
        $x = [double][C1Bits]::UInt32BitsToSingle($bits)
        foreach ($mode in 0..3) {
            $reference = ReferenceLogBits $x 24 $base $mode
            '{0x' + $bits.ToString('X') + 'U,0x' + $reference.ToString('X') + 'U,' + $mode + '}'
        }
    }
    $oracle = $prefix + 'static struct R{unsigned int x,y;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{float f;unsigned int u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].x;y.f=' + $function + 'f(x.f);if(y.u!=rows[i].y)return i+1;}return 0;}'
    Check ($function + '-fixed-point-44-float-directed') $oracle 0
}
