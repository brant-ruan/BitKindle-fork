
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
# Test-only exact integer oracle. No transcendental approximation is involved.
function IntegerPowReference([double]$x, [int]$n, [int]$precision, [int]$mode) {
    $raw = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
    $field = [int](($raw -shr 52) -band 2047)
    $m = [bigint]($raw -band ([Convert]::ToUInt64('fffffffffffff', 16)))
    if ($field) {
        $m += [bigint]::One -shl 52
    }
    else {
        $field = 1
    }
    $scale = ($field - 1023 - 52) * $n
    $exact = [bigint]::Pow($m, $n)
    $negative = $x -lt 0 -and ($n % 2 -eq 1)
    if ($negative) {
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
    $top = [int][C1Bits]::GetBitLength($exact) - 1
    $e = $top + $scale
    $field = $e + $bias
    $infinity = ([bigint]$maximum) -shl $p
    $inexact = $false
    $underflow = $false
    $overflow = $false
    if ($field -ge $maximum) {
        $overflow = $true
        $inexact = $true
        $out = if ($mode -lt 2) {
            $infinity
        }
        else {
            $infinity - 1
        }
    }
    else {
        $shift = [Math]::Max($top - $p, 1 - $bias - $p - $scale)
        $out = $exact -shr $shift
        $rest = $exact - ($out -shl $shift)
        if ($shift -le 0) {
            $rest = [bigint]::Zero
        }
        $inexact = !$rest.IsZero
        if ($mode -eq 1 -and $inexact) {
            $out += [bigint]::One
        }
        if ($mode -eq 0 -and $shift -gt 0) {
            $half = [bigint]::One -shl ($shift - 1)
            if ($rest -gt $half -or ($rest -eq $half -and !$out.IsEven)) {
                $out += [bigint]::One
            }
        }
        if ($field -gt 0) {
            $out += ([bigint]($field - 1)) -shl $p
        }
        if ($out -ge $infinity) {
            $overflow = $true
            $inexact = $true
        }
        $underflow = $out -lt ([bigint]::One -shl $p) -and $inexact
    }
    $bits = [uint64]$out
    if ($negative) {
        $bits = $bits -bor $(if ($precision -eq 24) {
                ([Convert]::ToUInt64('80000000', 16))
            }
            else {
                ([Convert]::ToUInt64('8000000000000000', 16))
            })
    }
    $flags = if ($overflow) {
        'FE_OVERFLOW|FE_INEXACT'
    }
    elseif ($underflow) {
        'FE_UNDERFLOW|FE_INEXACT'
    }
    elseif ($inexact) {
        'FE_INEXACT'
    }
    else {
        '0'
    }
    return @($bits, $flags, $(if ($overflow -or $underflow) {
                'ERANGE'
            }
            else {
                '0'
            }))
}
foreach ($precision in 53, 24) {
    $inputs = @(3, -3, 5, 7, 11, -11, 1.5, 0.75, 1.125, -1.125, 1.0000000000000002, 1.0000001192092896)
    if ($precision -eq 53) {
        $inputs += @([double]::MaxValue, [C1Math]::ScaleB(1.5, -530), [C1Math]::ScaleB(1.5, -358))
    }
    else {
        $inputs += @([double][float]::MaxValue, [C1Math]::ScaleB(1.5, -70), [C1Math]::ScaleB(1.5, -49))
    }
    $suffix = if ($precision -eq 53) {
        'ULL'
    }
    else {
        'U'
    }
    $rows = foreach ($x in $inputs) {
        if ($precision -eq 24) {
            $x = [double][float]$x
        }
        foreach ($n in 2, 3, 7, 17, 19, 34, 63, 64) {
            foreach ($mode in 0..3) {
                $ref = IntegerPowReference $x $n $precision $mode
                $a = if ($precision -eq 53) {
                    [C1Bits]::DoubleToUInt64Bits($x)
                }
                else {
                    [C1Bits]::SingleToUInt32Bits([float]$x)
                }
                '{0x' + $a.ToString('X') + $suffix + ',0x' + $ref[0].ToString('X') + $suffix + ',' + $n + ',' + $mode + ',' + $ref[1] + ',' + $ref[2] + '}'
            }
        }
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
        'pow'
    }
    else {
        'powf'
    }
    $extra = if ($precision -eq 53) {
        'feclearexcept(FE_ALL_EXCEPT);errno=0;r.f=(double)powl((long double)x.f,(long double)rows[i][2]);if(r.u!=rows[i][1]||fetestexcept(FE_ALL_EXCEPT)!=rows[i][4]||errno!=rows[i][5])return 2;'
    }
    else {
        ''
    }
    Check "pow-exact-integer-oracle-$precision" ($prefix + 'static ' + $integer + ' rows[][6]={' + ($rows -join ',') + '};int main(void){int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i;union{' + $type + ' f;' + $integer + ' u;}x,r;for(i=0;i<' + $rows.Count + ';i++){x.u=rows[i][0];fesetround(modes[rows[i][3]]);feclearexcept(FE_ALL_EXCEPT);errno=0;r.f=' + $fn + '(x.f,rows[i][2]);if(r.u!=rows[i][1]||fetestexcept(FE_ALL_EXCEPT)!=rows[i][4]||errno!=rows[i][5])return 1;' + $extra + '}return 0;}') 0
}
