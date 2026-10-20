
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Test oracle only. Half-angle reduction uses integer square roots, avoiding
# the implementation's pi constants and reciprocal/pi-over-four reduction.
$script:atanOracleFraction = 1400
$script:atanOracleUnit = [bigint]::One -shl $script:atanOracleFraction
function AtanIntegerRoot([bigint]$value) {
    $root = [bigint]::One -shl ([int][C1Math]::Ceiling([C1Bits]::GetBitLength($value) / 2.0))
    while ($true) {
        $next = [bigint]::Divide($root + [bigint]::Divide($value, $root), 2)
        if ($next -ge $root) {
            return $root
        }
        $root = $next
    }
}
function ReferenceAtanBits([double]$x, [int]$precision, [int]$mode) {
    $unit = $script:atanOracleUnit
    $negative = $x -lt 0
    $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
    $field = [int](($bits -shr 52) -band 2047)
    $mantissa = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $mantissa += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $argument = $mantissa -shl ($field - 1023 - 52 + $script:atanOracleFraction)
    $halves = 0
    while ($argument -gt ($unit -shr 3)) {
        $root = AtanIntegerRoot ($unit * $unit + $argument * $argument)
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
    $sum = $sum -shl $halves
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
    $exponent = [int][C1Bits]::GetBitLength($sum) - 1 - $script:atanOracleFraction
    $shift = $script:atanOracleFraction + $exponent - $p
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
foreach ($precision in 53, 24) {
    $inputs = @([C1Math]::ScaleB(1, -60), [C1Math]::ScaleB(1, -27), 0.41421356237309503, 0.4142135623730951, 0.5, 0.9999999999999999, 1.0, 1.0000000000000002, 2.0, [C1Math]::ScaleB(1, 60))
    $inputs += if ($precision -eq 53) {
        1e300
    }
    else {
        1e30
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
                $reference = ReferenceAtanBits $x $precision $mode
                $bits = if ($precision -eq 53) {
                    [C1Bits]::DoubleToUInt64Bits($x)
                }
                else {
                    [C1Bits]::SingleToUInt32Bits([float]$x)
                }
                '{0x' + $bits.ToString('X') + $suffix + ',0x' + $reference.ToString('X') + $suffix + ',' + $mode + '}'
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
    $fn = if ($precision -eq 53) {
        'atan'
    }
    else {
        'atanf'
    }
    $oracle = $prefix + 'static struct R{' + $integer + ' x,y;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].x;y.f=' + $fn + '(x.f);if(y.u!=rows[i].y)return i+1;}return 0;}'
    Check ('atan-fixed-point-' + $type + '-88-directed') $oracle 0
}
