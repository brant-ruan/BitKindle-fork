param([int]$DecimalPlaces = 600)
. (Join-Path $PSScriptRoot '../base/ps51-compat.ps1')

$ErrorActionPreference = 'Stop'
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
function LogSeries($num, $den) {
    $z = [Numerics.BigInteger]::Divide(($num - $den) * $scale, $num + $den)
    $square = [Numerics.BigInteger]::Divide($z * $z, $scale)
    $term = $z
    $sum = $z
    $i = 3
    while (!$term.IsZero) {
        $term = [Numerics.BigInteger]::Divide($term * $square, $scale)
        $sum += [Numerics.BigInteger]::Divide($term, $i)
        $i += 2
    }
    return 2 * $sum
}
$ln2 = LogSeries ([Numerics.BigInteger]2) ([Numerics.BigInteger]1)
function LogFixed($x) {
    $x = [Numerics.BigInteger]::Abs($x)
    $ex = [int]([C1Bits]::GetBitLength($x) - [C1Bits]::GetBitLength($scale))
    $num = $x
    $den = $scale
    if ($ex -ge 0) {
        $den = $den -shl $ex
    }
    else {
        $num = $num -shl (-$ex)
    }
    return (LogSeries $num $den) + $ex * $ln2
}
function Rounded($value, $p, $bias, $mode) {
    $neg = $value.Sign -lt 0
    $value = [Numerics.BigInteger]::Abs($value)
    $sign = [uint64]1 -shl $(if ($p -eq 23) {
            31
        }
        else {
            63
        })
    $inf = [uint64](2 * $bias + 1) -shl $p
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
    if ($quantum + $p -gt $bias) {
        $bits = if ($mode -eq 0 -or ($mode -eq 1 -and $neg) -or ($mode -eq 2 -and !$neg)) {
            $inf
        }
        else {
            $inf - 1
        }
    }
    elseif ($q -lt $lead) {
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
for ($h = -359; $h -le 343; $h += 2) {
    $x = $h / 2.0
    $num = $root
    $den = [Numerics.BigInteger]::One
    if ($h -gt 1) {
        for ($j = 1; $j -lt $h; $j += 2) {
            $num *= $j
            $den *= 2
        }
    }
    elseif ($h -lt 1) {
        for ($j = -1; $j -ge $h; $j -= 2) {
            $num *= 2
            $den *= $j
        }
    }
    $gamma = [Numerics.BigInteger]::Divide($num, $den)
    $log = LogFixed $gamma
    foreach ($p in 23, 52) {
        foreach ($op in 0, 1) {
            $value = if ($op) {
                $log
            }
            else {
                $gamma
            }
            $results = @()
            foreach ($mode in 0..3) {
                $bits = Rounded $value $p $(if ($p -eq 23) {
                        127
                    }
                    else {
                        1023
                    }) $mode
                $results += '0x' + $bits.ToString('x') + 'ULL'
            }
            $rows += ('{' + $p + ',' + $op + ',' + $x.ToString('R', [Globalization.CultureInfo]::InvariantCulture) + ',{' + ($results -join ',') + '}}')
        }
    }
}
# Near 1/2 and negative integer poles: independent local Gamma expansion.
$euler = [Numerics.BigInteger]::Parse('57721566490153286060651209008240243104215933593992') * [Numerics.BigInteger]::Pow(10, $DecimalPlaces - 50)
$second = [Numerics.BigInteger]::Divide($euler * $euler, 2 * $scale) + [Numerics.BigInteger]::Divide($pi * $pi, 12 * $scale)
foreach ($p in 23, 52) {
    foreach ($direction in -1, 1) {
        $shift = if ($p -eq 23) {
            21
        }
        else {
            50
        }
        $delta = ($scale -shr $shift) * $direction
        $dx = $direction * [Math]::Pow(2, - $shift)
        $g1 = $scale - [Numerics.BigInteger]::Divide($euler * $delta, $scale) + [Numerics.BigInteger]::Divide($second * $delta * $delta, $scale * $scale)
        foreach ($center in -2, 1, 2) {
            $x = $center + $dx
            $g = $g1
            if ($center -eq 2) {
                $g = [Numerics.BigInteger]::Divide($g * ($scale + $delta), $scale)
            }
            if ($center -eq -2) {
                $g = [Numerics.BigInteger]::Divide($g * $scale * $scale * $scale, ($delta - 2 * $scale) * ($delta - $scale) * $delta)
            }
            $log = LogFixed $g
            foreach ($op in 0, 1) {
                $value = if ($op) {
                    $log
                }
                else {
                    $g
                }
                $results = @()
                foreach ($mode in 0..3) {
                    $bits = Rounded $value $p $(if ($p -eq 23) {
                            127
                        }
                        else {
                            1023
                        }) $mode
                    $results += '0x' + $bits.ToString('x') + 'ULL'
                }
                $rows += ('{' + $p + ',' + $op + ',' + $x.ToString('R', [Globalization.CultureInfo]::InvariantCulture) + ',{' + ($results -join ',') + '}}')
            }
        }
    }
}
$body = @'
/* Test-only BigInteger half-integer Gamma recurrence and independent log series. */
static int gamma_reference(int mode){
struct V{int p,op;double x;unsigned long long r[4];};static struct V v[]={ROWS};
union{double f;unsigned long long u;}r;union{float f;unsigned int u;}rf;union{long double f;unsigned long long u;}rl;int i;
for(i=0;i<sizeof v/sizeof v[0];i++){
 if(v[i].p==23){rf.f=v[i].op?lgammaf((float)v[i].x):tgammaf((float)v[i].x);if(rf.u!=v[i].r[mode])return i+1;}
 else{r.f=v[i].op?lgamma(v[i].x):tgamma(v[i].x);rl.f=v[i].op?lgammal(v[i].x):tgammal(v[i].x);if(r.u!=v[i].r[mode]||rl.u!=v[i].r[mode])return i+1;}
}return 0;}
'@
Set-Content "$PSScriptRoot/gamma-reference.c" $body.Replace('ROWS', ($rows -join ','))
"Generated $($rows.Count) gamma reference records in all four modes"
