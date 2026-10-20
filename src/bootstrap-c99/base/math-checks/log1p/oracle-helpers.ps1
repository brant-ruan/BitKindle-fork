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
