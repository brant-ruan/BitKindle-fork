
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Test-only fixed-point reference: 1400 fractional bits, integer square root,
# half-angle arctangent. No production floating constants are used.
$script:inverseOracleFraction = 1400
$script:inverseOracleUnit = [bigint]::One -shl $script:inverseOracleFraction
function InverseIntegerRoot([bigint]$value) {
    if ($value.IsZero) {
        return [bigint]::Zero
    }
    $root = [bigint]::One -shl ([int][C1Math]::Ceiling([C1Bits]::GetBitLength($value) / 2.0))
    while ($true) {
        $next = [bigint]::Divide($root + [bigint]::Divide($value, $root), 2)
        if ($next -ge $root) {
            return $root
        }
        $root = $next
    }
}
function InverseFixedAtan([bigint]$argument) {
    $unit = $script:inverseOracleUnit
    $halves = 0
    while ($argument -gt ($unit -shr 3)) {
        $root = InverseIntegerRoot ($unit * $unit + $argument * $argument)
        $argument = [bigint]::Divide($argument * $unit, $unit + $root)
        $halves++
    }
    $square = - [bigint]::Divide($argument * $argument, $unit)
    $term = $argument
    $sum = $argument
    for ($n = 3; $n -lt 5000; $n += 2) {
        $term = [bigint]::Divide($term * $square, $unit)
        if ($term.IsZero) {
            break
        }
        $sum += [bigint]::Divide($term, $n)
    }
    return $sum -shl $halves
}
$script:inverseOraclePi2 = (InverseFixedAtan $script:inverseOracleUnit) * 2
function ReferenceInverseBits([double]$x, [int]$precision, [string]$fn, [int]$mode) {
    $unit = $script:inverseOracleUnit
    $negative = $x -lt 0 -and $fn -eq 'asin'
    $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
    $field = [int](($bits -shr 52) -band 2047)
    $mantissa = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $mantissa += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $argument = $mantissa -shl ($field - 1023 - 52 + $script:inverseOracleFraction)
    $root = InverseIntegerRoot ($unit * $unit - $argument * $argument)
    $sum = if ($root.IsZero) {
        $script:inverseOraclePi2
    }
    else {
        InverseFixedAtan ([bigint]::Divide($argument * $unit, $root))
    }
    if ($fn -eq 'acos') {
        $sum = if ($x -lt 0) {
            $script:inverseOraclePi2 + $sum
        }
        else {
            $script:inverseOraclePi2 - $sum
        }
    }
    if ($sum.IsZero) {
        return ([uint64]0)
    }
    if ($negative) {
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
    $exponent = [int][C1Bits]::GetBitLength($sum) - 1 - $script:inverseOracleFraction
    $shift = $script:inverseOracleFraction + $exponent - $p
    $q = $sum -shr $shift
    $rest = $sum - ($q -shl $shift)
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
foreach ($fn in 'asin', 'acos') {
    foreach ($precision in 53, 24) {
        $inputs = @(0.0, [C1Math]::ScaleB(1, -60), 0.1, 0.49999999999999994, 0.5, 0.5000000000000001, 0.9, 0.9999999999999999, 1.0)
        if ($precision -eq 24) {
            $inputs = @(0.0, [C1Math]::ScaleB(1, -60), 0.1, 0.4999999701976776, 0.5, 0.5000000596046448, 0.9, 0.9999999403953552, 1.0)
        }
        $suffix = if ($precision -eq 53) {
            'ULL'
        }
        else {
            'U'
        }
        $rows = foreach ($v in $inputs) {
            foreach ($sign in 1, -1) {
                $x = $v * $sign
                if ($precision -eq 24) {
                    $x = [double][float]$x
                }
                foreach ($mode in 0..3) {
                    if ($x -eq 0 -and $fn -eq 'asin') {
                        $expected = ([uint64]0)
                    }
                    else {
                        $expected = ReferenceInverseBits $x $precision $fn $mode
                    }
                    # The zero pairs use positive zero; signed zero is checked separately.
                    if ($x -eq 0) {
                        $x = 0.0
                    }
                    $bits = if ($precision -eq 53) {
                        [C1Bits]::DoubleToUInt64Bits($x)
                    }
                    else {
                        [C1Bits]::SingleToUInt32Bits([float]$x)
                    }
                    '{0x' + $bits.ToString('X') + $suffix + ',0x' + $expected.ToString('X') + $suffix + ',' + $mode + '}'
                }
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
        $call = if ($precision -eq 53) {
            $fn
        }
        else {
            $fn + 'f'
        }
        $oracle = $prefix + 'static struct R{' + $integer + ' x,y;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].x;y.f=' + $call + '(x.f);if(y.u!=rows[i].y)return i+1;}return 0;}'
        Check ($fn + '-fixed-point-' + $type + '-72-directed') $oracle 0
    }
}
