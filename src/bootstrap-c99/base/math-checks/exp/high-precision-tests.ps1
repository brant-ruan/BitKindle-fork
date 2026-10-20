
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Test oracle only: fixed-point arithmetic at 1400 fractional bits, using
# repeated squaring rather than the implementation's log(2) reduction.
function ReferenceExpBits([double]$value, [int]$precision, [int]$mode) {
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
    return [uint64]$q
}
$cases = @(-745.1332191019411, -745.0, -744.4, -710.0, -708.3964185322641, -1.0, -0.34657359027997264, -0.1, 0.1, 0.34657359027997264, 1.0, 10.0, 709.0, 709.782712893384, 709.7827128933841)
$rows = foreach ($x in $cases) {
    foreach ($mode in 0..3) {
        $bits = [C1Bits]::DoubleToUInt64Bits($x)
        $expected = ReferenceExpBits $x 53 $mode
        '{0x' + $bits.ToString('X') + 'ULL,0x' + $expected.ToString('X') + 'ULL,' + $mode + '}'
    }
}
$oracle = $prefix + 'static struct R{unsigned long long input,expected;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{double f;unsigned long long u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].input;y.f=exp(x.f);if(y.u!=rows[i].expected)return i+1;}return 0;}'
Check 'exp-fixed-point-60-directed' $oracle 0
$cases = @(-103.97, -103.0, -100.0, -87.33654, -1.0, -0.1, 0.1, 1.0, 80.0, 88.7, 88.722839)
$rows = foreach ($v in $cases) {
    $x = [float]$v
    foreach ($mode in 0..3) {
        $bits = [C1Bits]::SingleToUInt32Bits($x)
        $expected = ReferenceExpBits ([double]$x) 24 $mode
        '{0x' + $bits.ToString('X') + 'U,0x' + $expected.ToString('X') + 'U,' + $mode + '}'
    }
}
$oracle = $prefix + 'static struct R{unsigned int input,expected;int mode;} rows[]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{float f;unsigned int u;}x,y;for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++){fesetround(modes[rows[i].mode]);x.u=rows[i].input;y.f=expf(x.f);if(y.u!=rows[i].expected)return i+1;}return 0;}'
Check 'exp-fixed-point-44-float-directed' $oracle 0
