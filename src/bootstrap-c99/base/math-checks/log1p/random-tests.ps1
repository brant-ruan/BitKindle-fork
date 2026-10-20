
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
$rng = [Random]::new(172941)
foreach ($precision in 53, 24) {
    $suffix = if ($precision -eq 53) {
        'ULL'
    }
    else {
        'U'
    }
    $rows = for ($i = 0; $i -lt 1024; $i++) {
        if ($precision -eq 53) {
            $a = ([uint64]$rng.Next(0, $(if ($i -band 1) {
                            1023
                        }
                        else {
                            2047
                        })) -shl 52) -bor ([uint64]$rng.Next(0, 1048576) -shl 32) -bor [uint64]$rng.Next(1, 2147483647)
            if ($i -band 1) {
                $a = $a -bor ([Convert]::ToUInt64('8000000000000000', 16))
            }
            $x = [C1Bits]::UInt64BitsToDouble($a)
            if ($i -band 2) {
                $x = $rng.NextDouble() * 101 - 0.999999
                $a = [C1Bits]::DoubleToUInt64Bits($x)
            }
        }
        else {
            $a = ([uint32]$rng.Next(0, $(if ($i -band 1) {
                            127
                        }
                        else {
                            255
                        })) -shl 23) -bor [uint32]$rng.Next(1, 8388608)
            if ($i -band 1) {
                $a = $a -bor ([Convert]::ToUInt32('80000000', 16))
            }
            $x = [C1Bits]::UInt32BitsToSingle($a)
            if ($i -band 2) {
                $x = [float]($rng.NextDouble() * 101 - 0.999999)
                $a = [C1Bits]::SingleToUInt32Bits($x)
            }
        }
        $b = ReferenceLog1p ([double]$x) $precision 0
        '{0x' + $a.ToString('X') + $suffix + ',0x' + $b.ToString('X') + $suffix + '}'
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
    $fn = if ($precision -eq 53) {
        'log1p'
    }
    else {
        'log1pf'
    }
    Check "log1p-fixed-random-$precision" ($prefix + 'static ' + $integer + ' rows[][2]={' + ($rows -join ',') + '};int main(void){int i;union{' + $type + ' f;' + $integer + ' u;}x,r;for(i=0;i<1024;i++){x.u=rows[i][0];r.f=' + $fn + '(x.f);if(r.u!=rows[i][1])return 1;}return 0;}') 0
}
