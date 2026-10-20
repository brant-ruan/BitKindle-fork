
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
. (Join-Path $PSScriptRoot 'oracle-helpers.ps1')
# Test only: pi from half-angle atan(1), then fixed-point division and Taylor.
# This never writes a production constant or changes a compiler artifact.
function TrigReference([double]$x, [int]$kind, [int]$precision, [int]$mode) {
    $unit = $script:inverseOracleUnit
    $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
    $field = [int](($bits -shr 52) -band 2047)
    $m = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16))) + ([bigint]::One -shl 52)
    $fixed = $m -shl ($field - 1023 - 52 + $script:inverseOracleFraction)
    $pi2 = $script:inverseOraclePi2
    $n = [bigint]::Divide($fixed + ($pi2 -shr 1), $pi2)
    $r = $fixed - $n * $pi2
    $q = [int]($n % 4)
    $z = - [bigint]::Divide($r * $r, $unit)
    $s = $r
    $term = $r
    for ($i = 3; $i -lt 1000; $i += 2) {
        $term = [bigint]::Divide($term * $z, $unit * $i * ($i - 1))
        $s += $term
        if ($term.IsZero) {
            break
        }
    }
    $c = $unit
    $term = $unit
    for ($i = 2; $i -lt 1000; $i += 2) {
        $term = [bigint]::Divide($term * $z, $unit * $i * ($i - 1))
        $c += $term
        if ($term.IsZero) {
            break
        }
    }
    $sn = switch ($q) {
        0 {
            $s
        }1 {
            $c
        }2 {
            - $s
        }3 {
            - $c
        }
    }
    $cs = switch ($q) {
        0 {
            $c
        }1 {
            - $s
        }2 {
            - $c
        }3 {
            $s
        }
    }
    if ($x -lt 0) {
        $sn = - $sn
    }
    $sum = switch ($kind) {
        0 {
            $sn
        }1 {
            $cs
        }2 {
            [bigint]::Divide($sn * $unit, $cs)
        }
    }
    $negative = $sum.Sign -lt 0
    $sum = [bigint]::Abs($sum)
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
    $e = [int][C1Bits]::GetBitLength($sum) - 1 - $script:inverseOracleFraction
    $shift = $script:inverseOracleFraction + $e - $p
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
    $out += ([bigint]($e + $bias - 1)) -shl $p
    $v = [uint64]$out
    if ($negative) {
        $v = $v -bor $(if ($precision -eq 24) {
                ([Convert]::ToUInt64('80000000', 16))
            }
            else {
                ([Convert]::ToUInt64('8000000000000000', 16))
            })
    }
    return $v
}
foreach ($precision in 53, 24) {
    $inputs = @(0.1, -0.1, 0.7853981633974483, 1, 1.5707963267948966, 1.5707963267948963, 1.5707963267948968, 3.141592653589793, -10, 100, 1e10, 1e20, 1e30)
    if ($precision -eq 53) {
        $inputs += @(1e100, 1e200, 1e300, [double]::MaxValue)
    }
    else {
        $inputs += @([float]::MaxValue)
    }
    foreach ($kind in 0..2) {
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
                $expected = TrigReference $x $kind $precision $mode
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
        $fn = @('sin', 'cos', 'tan')[$kind] + $(if ($precision -eq 24) {
                'f'
            }
            else {
                ''
            })
        Check "trig-$fn-high-precision-directed" ($prefix + 'static ' + $integer + ' rows[][3]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,r;for(i=0;i<' + $rows.Count + ';i++){x.u=rows[i][0];fesetround(modes[rows[i][2]]);r.f=' + $fn + '(x.f);if(r.u!=rows[i][1])return i+1;}return 0;}') 0
    }
}
