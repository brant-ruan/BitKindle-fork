
. (Join-Path $PSScriptRoot '../base/ps51-compat.ps1')
$ErrorActionPreference = 'Stop'
$rng = [Random]::new(20260921)
$rows = @()
function Decode([uint64]$bits, $p, $bias) {
    $lead = [Numerics.BigInteger]::One -shl $p
    $field = [int](($bits -shr $p) -band (2 * $bias + 1))
    $m = [Numerics.BigInteger]($bits -band [uint64]($lead - 1))
    if ($field) {
        $m += $lead
    }
    else {
        $field = 1
    }
    if ($bits -band ([uint64]1 -shl ($p + $(if ($p -eq 23) {
                        8
                    }
                    else {
                        11
                    })))) {
        $m = - $m
    }
    return @($m, ($field - $bias - $p))
}
foreach ($p in 23, 52) {
    $bias = if ($p -eq 23) {
        127
    }
    else {
        1023
    }
    $lead = [Numerics.BigInteger]::One -shl $p
    $sign = [uint64]1 -shl $(if ($p -eq 23) {
            31
        }
        else {
            63
        })
    $inf = [uint64](2 * $bias + 1) -shl $p
    for ($i = 0; $i -lt 240; $i++) {
        $inputs = @()
        for ($j = 0; $j -lt 3; $j++) {
            $f = $rng.Next(0, 2 * $bias + 1)
            if ($i % 7 -eq 0) {
                $f = 0
            }
            $bits = ([uint64]$f -shl $p) -bor [uint64][C1Bits]::NextInt64($rng, 1, [long]$lead)
            if ($rng.Next(2)) {
                $bits = $bits -bor $sign
            }
            $inputs += $bits
        }
        $a = Decode $inputs[0] $p $bias
        $b = Decode $inputs[1] $p $bias
        $c = Decode $inputs[2] $p $bias
        $pe = $a[1] + $b[1]
        $e = [Math]::Min($pe, $c[1])
        $exact = (($a[0] * $b[0]) -shl ($pe - $e)) + ($c[0] -shl ($c[1] - $e))
        $neg = $exact.Sign -lt 0
        $mag = [Numerics.BigInteger]::Abs($exact)
        $results = @()
        $flags = @()
        foreach ($mode in 0..3) {
            if ($mag.IsZero) {
                $bits = if ($mode -eq 1) {
                    $sign
                }
                else {
                    [uint64]0
                }
                $flag = '0'
            }
            else {
                $h = [int][C1Bits]::GetBitLength($mag) - 1 + $e
                $quantum = [Math]::Max($h - $p, 1 - $bias - $p)
                $cut = $quantum - $e
                $rem = [Numerics.BigInteger]::Zero
                $den = [Numerics.BigInteger]::One
                if ($cut -gt 0) {
                    $den = $den -shl $cut
                    $q = [Numerics.BigInteger]::DivRem($mag, $den, [ref]$rem)
                }
                else {
                    $q = $mag -shl (-$cut)
                }
                $lost = !$rem.IsZero
                if ($lost -and (($mode -eq 0 -and (2 * $rem -gt $den -or (2 * $rem -eq $den -and !$q.IsEven))) -or ($mode -eq 1 -and $neg) -or ($mode -eq 2 -and !$neg))) {
                    $q += [Numerics.BigInteger]::One
                }
                if ($q -ge 2 * $lead) {
                    $q = $q -shr 1
                    $quantum++
                }
                $h = $quantum + $p
                $flag = if ($lost) {
                    'FE_INEXACT'
                }
                else {
                    '0'
                }
                if ($h -gt $bias) {
                    $flag = 'FE_OVERFLOW|FE_INEXACT'
                    $bits = if ($mode -eq 0 -or ($mode -eq 1 -and $neg) -or ($mode -eq 2 -and !$neg)) {
                        $inf
                    }
                    else {
                        $inf - 1
                    }
                }
                elseif ($q -lt $lead) {
                    $bits = [uint64]$q
                    if ($lost) {
                        $flag = 'FE_UNDERFLOW|FE_INEXACT'
                    }
                }
                else {
                    $bits = ([uint64]($h + $bias) -shl $p) -bor [uint64]($q - $lead)
                }
                if ($neg) {
                    $bits = $bits -bor $sign
                }
            }
            $results += '0x' + $bits.ToString('x') + 'ULL'
            $flags += $flag
        }
        $rows += ('{' + $p + ',' + (($inputs | ForEach-Object { '0x' + $_.ToString('x') + 'ULL' }) -join ',') + ',{' + ($results -join ',') + '},{' + ($flags -join ',') + '}}')
    }
}
$body = @'
/* Test-source oracle only: signed BigInteger exact sum and DivRem rounding. */
static int fma_reference(int mode){
struct V{int p;unsigned long long x,y,z,r[4];int flags[4];};struct V v[]={ROWS};
union{double f;unsigned long long u;}a,b,c,r;union{float f;unsigned int u;}af,bf,cf,rf;
union{long double f;unsigned long long u;}al,bl,cl,rl;int i;
for(i=0;i<sizeof v/sizeof v[0];i++){
 feclearexcept(FE_ALL_EXCEPT);errno=0;
 if(v[i].p==23){af.u=(unsigned int)v[i].x;bf.u=(unsigned int)v[i].y;cf.u=(unsigned int)v[i].z;rf.f=fmaf(af.f,bf.f,cf.f);if(rf.u!=v[i].r[mode])return i+1;}
 else{a.u=v[i].x;b.u=v[i].y;c.u=v[i].z;r.f=fma(a.f,b.f,c.f);al.u=a.u;bl.u=b.u;cl.u=c.u;rl.f=fmal(al.f,bl.f,cl.f);if(r.u!=v[i].r[mode]||rl.u!=v[i].r[mode])return i+1;}
 if(fetestexcept(FE_ALL_EXCEPT)!=v[i].flags[mode])return i+1;
 if(errno!=((v[i].flags[mode]&(FE_OVERFLOW|FE_UNDERFLOW))?ERANGE:0))return i+1;
}return 0;}
'@
Set-Content "$PSScriptRoot/fma-reference.c" $body.Replace('ROWS', ($rows -join ','))
"Generated $($rows.Count) exact FMA triples as C test source"
