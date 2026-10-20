
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
