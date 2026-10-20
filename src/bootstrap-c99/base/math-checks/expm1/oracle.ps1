
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Test oracle only: fixed-point arithmetic at 1400 fractional bits, using
# repeated squaring rather than the implementation's log(2) reduction.
function ReferenceExpm1Bits([double]$value, [int]$precision, [int]$mode) {
    $fractionBits = 1400
    $unit = [bigint]::One -shl $fractionBits
    $bits = [C1Bits]::DoubleToUInt64Bits($value)
    $field = [int](($bits -shr 52) -band 2047)
    $mantissa = [bigint]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $mantissa += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $argument = $mantissa -shl ($field - 1023 - 52 + $fractionBits)
    if ($bits -shr 63) {
        $argument = - $argument
    }
    $halves = 0
    while ([bigint]::Abs($argument) -gt ($unit -shr 3)) {
        $argument = [bigint]::Divide($argument, 2)
        $halves++
    }
    $sum = $unit
    $term = $unit
    for ($n = 1; $n -lt 512; $n++) {
        $term = [bigint]::Divide([bigint]::Multiply($term, $argument), [bigint]::Multiply($unit, $n))
        if ($term.IsZero) {
            break
        }
        $sum += $term
    }
    for ($n = 0; $n -lt $halves; $n++) {
        $sum = [bigint]::Divide([bigint]::Multiply($sum, $sum), $unit)
    }
    $sum -= $unit
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
    $maximum = 2 * $bias + 1
    $exponent = [int][C1Bits]::GetBitLength($sum) - 1 - $fractionBits
    $field = $exponent + $bias
    $infinity = ([bigint]$maximum) -shl $p
    if ($field -ge $maximum) {
        return [uint64]$(if ($mode -lt 2) {
                $infinity
            }
            else {
                $infinity - 1
            })
    }
    $shift = $fractionBits + [Math]::Max($exponent, 1 - $bias) - $p
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
    if ($field -gt 0) {
        $q += ([bigint]($field - 1)) -shl $p
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
