
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
. (Join-Path $PSScriptRoot 'oracle-helpers.ps1')
# Test-only 1400-bit fixed-point log and exp. Never emits production tables.
function PowFixedInput([double]$x) {
    $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
    $field = [int](($bits -shr 52) -band 2047)
    $m = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $m += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $r = $m -shl ($field - 1023 - 52 + $script:logOracleFraction)
    if ($x -lt 0) {
        $r = - $r
    }
    return $r
}
function PowReference([double]$x, [double]$y, [int]$precision, [int]$mode) {
    $unit = $script:logOracleUnit
    $fraction = $script:logOracleFraction
    $input = PowFixedInput ([Math]::Abs($x))
    $exponent = [int][C1Bits]::GetBitLength($input) - 1 - $fraction
    $normalized = $input -shr $exponent
    $log = (FixedNaturalLog $normalized) + $exponent * $script:logOracleLn2
    $argument = [bigint]::Divide($log * (PowFixedInput $y), $unit)
    $halves = 0
    while ([bigint]::Abs($argument) -gt ($unit -shr 3)) {
        $argument = [bigint]::Divide($argument, 2)
        $halves++
    }
    $sum = $unit
    $term = $unit
    for ($i = 1; $i -lt 512; $i++) {
        $term = [bigint]::Divide($term * $argument, $unit * $i)
        if ($term.IsZero) {
            break
        }
        $sum += $term
    }
    for ($i = 0; $i -lt $halves; $i++) {
        $sum = [bigint]::Divide($sum * $sum, $unit)
    }
    $negative = $x -lt 0 -and ([Math]::Abs($y) % 2 -eq 1)
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
    $maximum = 2 * $bias + 1
    $exponent = [int][C1Bits]::GetBitLength($sum) - 1 - $fraction
    $field = $exponent + $bias
    $infinity = ([bigint]$maximum) -shl $p
    if ($field -ge $maximum) {
        $out = if ($mode -lt 2) {
            $infinity
        }
        else {
            $infinity - 1
        }
    }
    else {
        $shift = $fraction + [Math]::Max($exponent, 1 - $bias) - $p
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
    $pairs = @(@(1.01, 100.3), @(0.99, -100.3), @(8, 0.3333333333333333), @(9, 0.1), @(2, -10.1), @(2, 10.1), @(0.1, 0.1), @(3, -2.3), @(-1.125, 17), @(-1.125, -17), @(1.0001, 10000.3), @(0.9999, 10000.3))
    if ($precision -eq 53) {
        $pairs += @(@(1.0000000000000002, 1e18), @(0.9999999999999999, 1e18), @(2, -1074.5), @(2, 1023.9), @(1e300, -0.9), @(1e-300, 0.9))
    }
    else {
        $pairs += @(@(1.0000001192092896, 1e8), @(0.9999999403953552, 1e8), @(2, -149.5), @(2, 127.9), @(1e30, -0.9), @(1e-30, 0.9))
    }
    $suffix = if ($precision -eq 53) {
        'ULL'
    }
    else {
        'U'
    }
    $rows = foreach ($pair in $pairs) {
        $x = $pair[0]
        $y = $pair[1]
        if ($precision -eq 24) {
            $x = [double][float]$x
            $y = [double][float]$y
        }
        foreach ($mode in 0..3) {
            $expected = PowReference $x $y $precision $mode
            $a = if ($precision -eq 53) {
                [C1Bits]::DoubleToUInt64Bits($x)
            }
            else {
                [C1Bits]::SingleToUInt32Bits([float]$x)
            }
            $b = if ($precision -eq 53) {
                [C1Bits]::DoubleToUInt64Bits($y)
            }
            else {
                [C1Bits]::SingleToUInt32Bits([float]$y)
            }
            '{0x' + $a.ToString('X') + $suffix + ',0x' + $b.ToString('X') + $suffix + ',0x' + $expected.ToString('X') + $suffix + ',' + $mode + '}'
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
        'pow'
    }
    else {
        'powf'
    }
    Check "pow-high-precision-$precision" ($prefix + 'static ' + $integer + ' rows[][4]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,y,r;for(i=0;i<' + $rows.Count + ';i++){x.u=rows[i][0];y.u=rows[i][1];fesetround(modes[rows[i][3]]);r.f=' + $fn + '(x.f,y.f);if(r.u!=rows[i][2])return i+1;}return 0;}') 0
}
