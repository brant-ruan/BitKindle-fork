
. (Join-Path $PSScriptRoot '../base/ps51-compat.ps1')
$ErrorActionPreference = 'Stop'
$rng = [Random]::new(20260919)
$rows = @()
foreach ($type in 13, 14) {
    $p = if ($type -eq 13) {
        23
    }
    else {
        52
    }
    $bias = if ($type -eq 13) {
        127
    }
    else {
        1023
    }
    $limit = 2 * $bias + 1
    $lead = [Numerics.BigInteger]::One -shl $p
    $sign = [uint64]1 -shl $(if ($type -eq 13) {
            31
        }
        else {
            63
        })
    for ($i = 0; $i -lt 160; $i++) {
        $fx = $rng.Next(0, $limit)
        $fy = $rng.Next(0, $limit)
        if ($i % 8 -eq 0) {
            $fx = 0
        }
        if ($i % 8 -eq 1) {
            $fy = 0
        }
        $mx = [Numerics.BigInteger][C1Bits]::NextInt64($rng, 0, [long]$lead)
        $my = [Numerics.BigInteger][C1Bits]::NextInt64($rng, 1, [long]$lead)
        $bx = ([uint64]$fx -shl $p) -bor [uint64]$mx
        $by = ([uint64]$fy -shl $p) -bor [uint64]$my
        $sx = $rng.Next(0, 2)
        $sy = $rng.Next(0, 2)
        if ($sx) {
            $bx = $bx -bor $sign
        }
        if ($sy) {
            $by = $by -bor $sign
        }
        if ($fx) {
            $mx += $lead
            $ex = $fx - $bias - $p
        }
        else {
            $ex = 1 - $bias - $p
        }
        if ($fy) {
            $my += $lead
            $ey = $fy - $bias - $p
        }
        else {
            $ey = 1 - $bias - $p
        }
        $scale = [Math]::Min($ex, $ey)
        $nx = $mx -shl ($ex - $scale)
        $ny = $my -shl ($ey - $scale)
        $q = [Numerics.BigInteger]::Divide($nx, $ny)
        $r = $nx - $q * $ny
        if (2 * $r -gt $ny -or (2 * $r -eq $ny -and !$q.IsEven)) {
            $q += [Numerics.BigInteger]::One
            $r -= $ny
        }
        if ($sx) {
            $r = - $r
        }
        $negative = $r.Sign -lt 0
        $r = [Numerics.BigInteger]::Abs($r)
        $bits = [uint64]0
        if (!$r.IsZero) {
            $len = [int][C1Bits]::GetBitLength($r)
            $e = $scale + $len - 1
            if ($e -ge 1 - $bias) {
                if ($len -gt $p + 1) {
                    $mant = $r -shr ($len - $p - 1)
                    if (($mant -shl ($len - $p - 1)) -ne $r) {
                        throw 'Oracle remainder was not exact'
                    }
                }
                else {
                    $mant = $r -shl ($p + 1 - $len)
                }
                $bits = ([uint64]($e + $bias) -shl $p) -bor [uint64]($mant - $lead)
            }
            else {
                $bits = [uint64]($r -shl ($scale + $bias + $p - 1))
            }
        }
        if ($negative -or ($r.IsZero -and $sx)) {
            $bits = $bits -bor $sign
        }
        $quo = [int]($q % 128)
        if ($sx -ne $sy) {
            $quo = - $quo
        }
        $rows += ('{' + $type + ',0x' + $bx.ToString('x') + 'ULL,0x' + $by.ToString('x') + 'ULL,0x' + $bits.ToString('x') + 'ULL,' + $quo + '}')
    }
}
$body = '/* Test-only exact rational oracle generated with host BigInteger arithmetic. */' + "`n" + 'static int remquo_reference(void){struct V{int t;unsigned long long x,y,r;int q;};struct V v[]={' + ($rows -join ',') + '};union{double f;unsigned long long u;}a,b,r;union{float f;unsigned int u;}af,bf,rf;int i,q;for(i=0;i<sizeof v/sizeof v[0];i++){if(v[i].t==13){af.u=(unsigned int)v[i].x;bf.u=(unsigned int)v[i].y;rf.f=remquof(af.f,bf.f,&q);if(rf.u!=v[i].r||q!=v[i].q)return i+1;}else{a.u=v[i].x;b.u=v[i].y;r.f=remquo(a.f,b.f,&q);if(r.u!=v[i].r||q!=v[i].q)return i+1;}}return 0;}'
Set-Content (Join-Path $PSScriptRoot 'remquo-reference.c') $body
"Generated $($rows.Count) exact reference cases as C test source"

