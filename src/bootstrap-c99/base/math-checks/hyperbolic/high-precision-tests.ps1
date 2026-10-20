
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Test-only high precision reference, independent of the C double-double code.
$script:hypOracleFraction = 1400
$script:hypOracleUnit = [bigint]::One -shl $script:hypOracleFraction
function HypFixedExp([double]$x) {
    $unit = $script:hypOracleUnit
    $bits = [C1Bits]::DoubleToUInt64Bits($x)
    $field = [int](($bits -shr 52) -band 2047)
    $mantissa = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $mantissa += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $argument = $mantissa -shl ($field - 1023 - 52 + $script:hypOracleFraction)
    $halves = 0
    while ($argument -gt ($unit -shr 3)) {
        $argument = [bigint]::Divide($argument, 2)
        $halves++
    }
    $term = $unit
    $sum = $unit
    for ($n = 1; $n -lt 512; $n++) {
        $term = [bigint]::Divide($term * $argument, $unit * $n)
        if ($term.IsZero) {
            break
        }
        $sum += $term
    }
    for ($n = 0; $n -lt $halves; $n++) {
        $sum = [bigint]::Divide($sum * $sum, $unit)
    }
    return $sum
}
function ReferenceHypBits([double]$x, [int]$precision, [string]$function, [int]$mode) {
    $unit = $script:hypOracleUnit
    $negative = $x -lt 0 -and $function -ne 'cosh'
    $positive = HypFixedExp ([Math]::Abs($x))
    $inverse = [bigint]::Divide($unit * $unit, $positive)
    $sum = $positive + $inverse
    $difference = $positive - $inverse
    $y = switch ($function) {
        'sinh' {
            [bigint]::Divide($difference, 2)
        }'cosh' {
            [bigint]::Divide($sum, 2)
        }'tanh' {
            [bigint]::Divide($difference * $unit, $sum)
        }
    }
    if ($negative) {
        if ($mode -eq 1) {
            $mode = 2
        }
        elseif ($mode -eq 2) {
            $mode = 1
        }
    }
    if ($y.IsZero) {
        return $(if ($negative) {
                if ($precision -eq 24) {
                    ([Convert]::ToUInt64('80000000', 16))
                }
                else {
                    ([Convert]::ToUInt64('8000000000000000', 16))
                }
            }
            else {
                ([uint64]0)
            })
    }
    $p = $precision - 1
    $bias = if ($precision -eq 24) {
        127
    }
    else {
        1023
    }
    $maximum = 2 * $bias + 1
    $exponent = [int][C1Bits]::GetBitLength($y) - 1 - $script:hypOracleFraction
    $field = $exponent + $bias
    $infinity = ([bigint]$maximum) -shl $p
    if ($field -ge $maximum) {
        $q = if ($mode -lt 2) {
            $infinity
        }
        else {
            $infinity - 1
        }
    }
    else {
        $shift = $script:hypOracleFraction + [Math]::Max($exponent, 1 - $bias) - $p
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
        if ($field -gt 0) {
            $q += ([bigint]($field - 1)) -shl $p
        }
    }
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
foreach ($fn in 'sinh', 'cosh', 'tanh') {
    $inputs = @( - [C1Math]::ScaleB(1, -60), [C1Math]::ScaleB(1, -60), -0.1, 0.1, -0.5, 0.5, -1.0, 1.0, -20.0, 20.0, 39.99999999999999, 40.0)
    if ($fn -ne 'tanh') {
        $inputs += @(-710.0, 710.0, 710.4758600739439)
    }
    $rows = foreach ($x in $inputs) {
        foreach ($mode in 0..3) {
            $expected = ReferenceHypBits $x 53 $fn $mode
            '{0x' + ([C1Bits]::DoubleToUInt64Bits($x)).ToString('X') + 'ULL,0x' + $expected.ToString('X') + 'ULL,' + $mode + '}'
        }
    }
    $oracle = $prefix + 'static struct R{unsigned long long x,y;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].x;y.f=' + $fn + '(x.f);if(y.u!=rows[i].y)return i+1;}return 0;}'
    Check ($fn + '-fixed-point-directed-double') $oracle 0
    $inputs = @( - [C1Math]::ScaleB(1, -30), [C1Math]::ScaleB(1, -30), -0.1, 0.1, -0.5, 0.5, -1.0, 1.0, -20.0, 20.0, 40.0)
    if ($fn -ne 'tanh') {
        $inputs += @(-89.0, 89.0, 89.415985)
    }
    $rows = foreach ($v in $inputs) {
        $x = [float]$v
        foreach ($mode in 0..3) {
            $expected = ReferenceHypBits ([double]$x) 24 $fn $mode
            '{0x' + ([C1Bits]::SingleToUInt32Bits($x)).ToString('X') + 'U,0x' + $expected.ToString('X') + 'U,' + $mode + '}'
        }
    }
    $oracle = $prefix + 'static struct R{unsigned int x,y;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{float f;unsigned int u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].x;y.f=' + $fn + 'f(x.f);if(y.u!=rows[i].y)return i+1;}return 0;}'
    Check ($fn + '-fixed-point-directed-float') $oracle 0
}
