
. (Join-Path $PSScriptRoot '../base/ps51-compat.ps1')
$ErrorActionPreference = 'Stop'
$rng = [Random]::new(20260920)
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
    $lead = [Numerics.BigInteger]::One -shl $p
    $sign = [uint64]1 -shl $(if ($type -eq 13) {
            31
        }
        else {
            63
        })
    for ($i = 0; $i -lt 160; $i++) {
        $field = $rng.Next(0, 2 * $bias + 1)
        if ($i % 8 -eq 0) {
            $field = 0
        }
        $m = [Numerics.BigInteger][C1Bits]::NextInt64($rng, 1, [long]$lead)
        if ($i % 8 -eq 1) {
            $m = [Numerics.BigInteger]::One
        }
        if ($i % 8 -eq 2) {
            $m = $lead - 1
        }
        $input = ([uint64]$field -shl $p) -bor [uint64]$m
        $neg = $rng.Next(0, 2)
        if ($neg) {
            $input = $input -bor $sign
        }
        if ($field) {
            $m += $lead
            $e = $field - $bias
        }
        else {
            $e = 1 - $bias
            while ($m -lt $lead) {
                $m = $m -shl 1
                $e--
            }
        }
        $k = [int][C1Math]::Floor($e / 3.0)
        $n = $m -shl (2 * $p + $e - 3 * $k)
        $q = [Numerics.BigInteger]::One -shl ([int][C1Math]::Ceiling([C1Bits]::GetBitLength($n) / 3.0))
        while ($true) {
            $next = [Numerics.BigInteger]::Divide(2 * $q + [Numerics.BigInteger]::Divide($n, $q * $q), 3)
            if ($next -ge $q) {
                break
            }
            $q = $next
        }
        if ($q * $q * $q -gt $n -or ($q + 1) * ($q + 1) * ($q + 1) -le $n) {
            throw 'Integer Newton root not bracketed'
        }
        $inexact = $q * $q * $q -ne $n
        $mid = 2 * $q + 1
        $upNearest = $mid * $mid * $mid -lt 8 * $n
        $result = @()
        foreach ($mode in 0..3) {
            $out = $q
            $exp = $k
            if ($inexact -and (($mode -eq 0 -and $upNearest) -or ($mode -eq 1 -and $neg) -or ($mode -eq 2 -and !$neg))) {
                $out += [Numerics.BigInteger]::One
            }
            if ($out -eq 2 * $lead) {
                $out = $out -shr 1
                $exp++
            }
            $bits = ([uint64]($exp + $bias) -shl $p) -bor [uint64]($out - $lead)
            if ($neg) {
                $bits = $bits -bor $sign
            }
            $result += '0x' + $bits.ToString('x') + 'ULL'
        }
        $rows += ('{' + $type + ',0x' + $input.ToString('x') + 'ULL,{' + ($result -join ',') + '},' + [int]$inexact + '}')
    }
}
$body = '/* Exact test oracle: host BigInteger Newton iteration plus midpoint comparison. */' + "`n" + 'static int cbrt_reference(int mode){struct V{int t;unsigned long long x,r[4];int inexact;};struct V v[]={' + ($rows -join ',') + '};union{double f;unsigned long long u;}a,r;union{float f;unsigned int u;}af,rf;union{long double f;unsigned long long u;}al,rl;int i;for(i=0;i<sizeof v/sizeof v[0];i++){feclearexcept(FE_ALL_EXCEPT);if(v[i].t==13){af.u=(unsigned int)v[i].x;rf.f=cbrtf(af.f);if(rf.u!=v[i].r[mode])return i+1;}else{a.u=v[i].x;r.f=cbrt(a.f);al.u=v[i].x;rl.f=cbrtl(al.f);if(r.u!=v[i].r[mode]||rl.u!=v[i].r[mode])return i+1;}if(!!fetestexcept(FE_INEXACT)!=v[i].inexact||fetestexcept(FE_INVALID|FE_UNDERFLOW|FE_OVERFLOW))return i+1;}return 0;}'
Set-Content (Join-Path $PSScriptRoot 'cbrt-reference.c') $body
"Generated $($rows.Count) exact cube-root reference inputs as C test source"
