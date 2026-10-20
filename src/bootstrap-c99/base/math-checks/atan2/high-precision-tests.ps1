
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
. (Join-Path $PSScriptRoot 'oracle-helpers.ps1')
function Atan2FixedInput([double]$x) {
    $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
    $field = [int](($bits -shr 52) -band 2047)
    $mantissa = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $mantissa += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    return $mantissa -shl ($field - 1023 - 52 + $script:inverseOracleFraction)
}
function ReferenceAtan2Bits([double]$y, [double]$x, [int]$precision, [int]$mode) {
    $unit = $script:inverseOracleUnit
    $ny = Atan2FixedInput $y
    $nx = Atan2FixedInput $x
    $sum = InverseFixedAtan ([bigint]::Divide($ny * $unit, $nx))
    if ($x -lt 0) {
        $sum = 2 * $script:inverseOraclePi2 - $sum
    }
    $negative = $y -lt 0
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
$pairs = @(@(1.0, 1.0), @(1.0, -1.0), @(-1.0, 1.0), @(-1.0, -1.0), @(1.0, 2.0), @(2.0, 1.0), @(-1.0, 2.0), @(1.0, -2.0), @(1.0, 0.41421356237309503), @(0.41421356237309503, 1.0), @([C1Math]::ScaleB(1, -60), 1.0), @([C1Math]::ScaleB(1, -61), 1.0), @([C1Math]::ScaleB(1, -100), 1.0), @(1.0, [C1Math]::ScaleB(1, -100)), @([C1Math]::ScaleB(1, -100), -1.0), @( - [C1Math]::ScaleB(1, -100), -1.0))
foreach ($precision in 53, 24) {
    $suffix = if ($precision -eq 53) {
        'ULL'
    }
    else {
        'U'
    }
    $rows = foreach ($pair in $pairs) {
        $y = $pair[0]
        $x = $pair[1]
        if ($precision -eq 24) {
            $y = [double][float]$y
            $x = [double][float]$x
        }
        foreach ($mode in 0..3) {
            $expected = ReferenceAtan2Bits $y $x $precision $mode
            $a = if ($precision -eq 53) {
                [C1Bits]::DoubleToUInt64Bits($y)
            }
            else {
                [C1Bits]::SingleToUInt32Bits([float]$y)
            }
            $b = if ($precision -eq 53) {
                [C1Bits]::DoubleToUInt64Bits($x)
            }
            else {
                [C1Bits]::SingleToUInt32Bits([float]$x)
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
        'atan2'
    }
    else {
        'atan2f'
    }
    $oracle = $prefix + 'static struct R{' + $integer + ' y,x,result;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}y,x,r;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);y.u=rows[i].y;x.u=rows[i].x;r.f=' + $fn + '(y.f,x.f);if(r.u!=rows[i].result)return i+1;}return 0;}'
    Check ('atan2-fixed-point-' + $type + '-64-directed') $oracle 0
}
