param([ValidateRange(800, 1200)][int]$DecimalPlaces = 800)
. (Join-Path $PSScriptRoot '../base/ps51-compat.ps1')

$ErrorActionPreference = 'Stop'
# Test source only: 800-decimal-place integer arithmetic, independent Taylor series.
$scale = [Numerics.BigInteger]::Pow(10, $DecimalPlaces)
function AtanReciprocal($d) {
    $power = [Numerics.BigInteger]::Divide($scale, $d)
    $sum = $power
    $n = 1
    while (!$power.IsZero) {
        $power = - [Numerics.BigInteger]::Divide($power, $d * $d)
        $sum += [Numerics.BigInteger]::Divide($power, 2 * $n + 1)
        $n++
    }
    return $sum
}
$pi = 16 * (AtanReciprocal 5) - 4 * (AtanReciprocal 239)
$n = $pi * $scale
$root = [Numerics.BigInteger]::One -shl ([int][C1Math]::Ceiling([C1Bits]::GetBitLength($n) / 2.0))
while ($true) {
    $next = ($root + [Numerics.BigInteger]::Divide($n, $root)) -shr 1
    if ($next -ge $root) {
        break
    }
    $root = $next
}
$coefficient = [Numerics.BigInteger]::Divide(2 * $scale * $scale, $root)
function Rounded($value, $p, $bias, $mode) {
    $neg = $value.Sign -lt 0
    $value = [Numerics.BigInteger]::Abs($value)
    $sign = [uint64]1 -shl $(if ($p -eq 23) {
            31
        }
        else {
            63
        })
    if ($value.IsZero) {
        return [uint64]0
    }
    $ex = [int]([C1Bits]::GetBitLength($value) - [C1Bits]::GetBitLength($scale))
    if ($ex -ge 0) {
        if ($value -lt ($scale -shl $ex)) {
            $ex--
        }
    }
    elseif (($value -shl (-$ex)) -lt $scale) {
        $ex--
    }
    $quantum = [Math]::Max($ex - $p, 1 - $bias - $p)
    $num = $value
    $den = $scale
    if ($quantum -lt 0) {
        $num = $num -shl (-$quantum)
    }
    else {
        $den = $den -shl $quantum
    }
    $rem = [Numerics.BigInteger]::Zero
    $q = [Numerics.BigInteger]::DivRem($num, $den, [ref]$rem)
    if (!$rem.IsZero -and (($mode -eq 0 -and (2 * $rem -gt $den -or (2 * $rem -eq $den -and !$q.IsEven))) -or ($mode -eq 1 -and $neg) -or ($mode -eq 2 -and !$neg))) {
        $q += [Numerics.BigInteger]::One
    }
    $lead = [Numerics.BigInteger]::One -shl $p
    if ($q -ge 2 * $lead) {
        $q = $q -shr 1
        $quantum++
    }
    if ($q -lt $lead) {
        $bits = [uint64]$q
    }
    else {
        $bits = ([uint64]($quantum + $p + $bias) -shl $p) -bor [uint64]($q - $lead)
    }
    if ($neg) {
        $bits = $bits -bor $sign
    }
    return $bits
}
$rows = @()
$inputs = @()
for ($i = -48; $i -le 224; $i++) {
    $inputs += ([double]$i / 8)
}
$inputs += 1.9999999999999998, 2.0000000000000004, 5.999999999999999, 26.5, 26.6, 26.7, 26.8, 27.1, 27.2, 27.25, 27.3
$inputs += [C1Bits]::UInt64BitsToDouble(1), [C1Bits]::UInt64BitsToDouble(([Convert]::ToUInt64('10000000000000', 16))), [Math]::Pow(2, -60), - [Math]::Pow(2, -61), [Math]::Pow(2, -149), [Math]::Pow(2, -126), 9.25, 9.5, 10.0, 10.125
foreach ($precision in 23, 52) {
    foreach ($original in $inputs) {
        $x = if ($precision -eq 23) {
            [double][float]$original
        }
        else {
            $original
        }
        $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
        $field = [int]($bits -shr 52)
        $mant = [Numerics.BigInteger]($bits -band ([Convert]::ToUInt64('fffffffffffff', 16)))
        if ($field) {
            $mant += [Numerics.BigInteger]::One -shl 52
        }
        else {
            $field = 1
        }
        $ex = $field - 1023 - 52
        $num = $mant
        $den = [Numerics.BigInteger]::One
        if ($ex -ge 0) {
            $num = $num -shl $ex
        }
        else {
            $den = $den -shl (-$ex)
        }
        $term = [Numerics.BigInteger]::Divide($num * $scale, $den)
        $sum = $term
        $squareNum = $num * $num
        $squareDen = $den * $den
        $j = 1
        while (!$term.IsZero) {
            $term = - [Numerics.BigInteger]::Divide($term * $squareNum, $squareDen * $j)
            $sum += [Numerics.BigInteger]::Divide($term, 2 * $j + 1)
            $j++
        }
        $value = [Numerics.BigInteger]::Divide($sum * $coefficient, $scale)
        if ($x -lt 0) {
            $value = - $value
        }
        foreach ($op in 0, 1) {
            $v = if ($op) {
                $scale - $value
            }
            else {
                $value
            }
            $results = @()
            foreach ($mode in 0..3) {
                $out = Rounded $v $precision $(if ($precision -eq 23) {
                        127
                    }
                    else {
                        1023
                    }) $mode
                $results += '0x' + $out.ToString('x') + 'ULL'
            }
            $input = [C1Bits]::DoubleToUInt64Bits($x)
            $rows += ('{' + $precision + ',' + $op + ',0x' + $input.ToString('x') + 'ULL,{' + ($results -join ',') + '}}')
        }
    }
}
$body = @'
/* Independent fixed-point reference generated from the defining Taylor series. */
static int erf_reference(int mode){
 struct V{int p,op;unsigned long long x,r[4];};struct V v[]={ROWS};
 union{double f;unsigned long long u;}x,r;union{long double f;unsigned long long u;}xl,rl;union{float f;unsigned int u;}rf;int i;
 for(i=0;i<sizeof v/sizeof v[0];i++){
  x.u=v[i].x;if(v[i].p==23){rf.f=v[i].op?erfcf((float)x.f):erff((float)x.f);if(rf.u!=v[i].r[mode])return i+1;continue;}xl.u=x.u;r.f=v[i].op?erfc(x.f):erf(x.f);rl.f=v[i].op?erfcl(xl.f):erfl(xl.f);
  if(r.u!=v[i].r[mode]||rl.u!=v[i].r[mode])return i+1;
 }return 0;
}
'@
Set-Content "$PSScriptRoot/erf-reference.c" $body.Replace('ROWS', ($rows -join ','))
"Generated $($rows.Count) erf/erfc reference cases in all four rounding modes"

