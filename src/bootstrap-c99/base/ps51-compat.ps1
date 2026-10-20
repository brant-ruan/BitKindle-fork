#requires -Version 5.1
# Test/build helpers only. No external compiler or binary code generation.
class C1Bits {
    static [uint64] ParseDecimalBits([string]$text, [int]$precision) {
        # Test-only exact rational decimal conversion, round to nearest/even.
        # CLR 4 Parse throws on overflow and is not correctly rounded for all
        # long inputs. Keep the same reference contract as modern .NET.
        $m = [regex]::Match($text.Trim(), '^(?<sign>[+-]?)(?<whole>[0-9]*)(?:\.(?<fraction>[0-9]*))?(?:[eE](?<exponent>[+-]?[0-9]+))?$')
        if (!$m.Success) {
            throw 'Invalid invariant decimal reference'
        }
        $digits = $m.Groups['whole'].Value + $m.Groups['fraction'].Value
        if (!$digits.Length) {
            throw 'Empty decimal reference'
        }
        $p = $precision - 1
        $bias = 1023
        if ($precision -eq 24) {
            $bias = 127
        }
        $sign = [uint64]0
        if ($m.Groups['sign'].Value -eq '-') {
            $sign = [uint64]1 -shl $(if ($precision -eq 24) {
                    31
                }
                else {
                    63
                })
        }
        $digits = $digits.TrimStart([char]'0')
        if (!$digits.Length) {
            return $sign
        }
        $power = - $m.Groups['fraction'].Value.Length
        if ($m.Groups['exponent'].Success) {
            $power += [int]$m.Groups['exponent'].Value
        }
        $inf = [uint64](2 * $bias + 1) -shl $p
        if ($power + $digits.Length -gt 400) {
            return $sign -bor $inf
        }
        if ($power + $digits.Length -lt -400) {
            return $sign
        }
        $num = [bigint]::Parse($digits, [Globalization.CultureInfo]::InvariantCulture)
        $den = [bigint]::One
        if ($power -ge 0) {
            $num *= [bigint]::Pow(10, $power)
        }
        else {
            $den = [bigint]::Pow(10, - $power)
        }
        $ex = [int]([C1Bits]::GetBitLength($num) - [C1Bits]::GetBitLength($den))
        if ($ex -ge 0) {
            if ($num -lt ($den -shl $ex)) {
                $ex--
            }
        }
        elseif (($num -shl (-$ex)) -lt $den) {
            $ex--
        }
        if ($ex -gt $bias) {
            return $sign -bor $inf
        }
        $quantum = [Math]::Max($ex - $p, 1 - $bias - $p)
        if ($quantum -lt 0) {
            $num = $num -shl (-$quantum)
        }
        else {
            $den = $den -shl $quantum
        }
        $q = [bigint]::Divide($num, $den)
        $rest = $num - $q * $den
        if (2 * $rest -gt $den -or (2 * $rest -eq $den -and !$q.IsEven)) {
            $q += [bigint]::One
        }
        if ($ex -ge 1 - $bias) {
            $q += [bigint]($ex + $bias - 1) -shl $p
        }
        return $sign -bor [uint64]$q
    }
    static [long] GetBitLength([bigint]$x) {
        if ($x.Sign -lt 0) {
            $x = - $x - 1
        }
        if ($x.IsZero) {
            return 0
        }
        $bytes = $x.ToByteArray()
        $i = $bytes.Length - 1
        while ($bytes[$i] -eq 0) {
            $i--
        }
        $bits = [long]$i * 8
        $top = [int]$bytes[$i]
        while ($top -gt 0) {
            $bits++
            $top = $top -shr 1
        }
        return $bits
    }
    static [long] NextInt64([Random]$random, [long]$min, [long]$max) {
        if ($min -lt 0 -or $max -le $min) {
            throw 'Reference generator requires 0 <= min < max'
        }
        $span = [uint64]($max - $min)
        $mask = [uint64]1
        while ($mask -lt $span - 1) {
            $mask = ($mask -shl 1) -bor [uint64]1
        }
        $bytes = [byte[]]::new(8)
        do {
            $random.NextBytes($bytes)
            $value = [BitConverter]::ToUInt64($bytes, 0) -band $mask
        }while ($value -ge $span)
        return [long]$value + $min
    }
    static [uint64] DoubleToUInt64Bits([double]$x) {
        return [BitConverter]::ToUInt64([BitConverter]::GetBytes($x), 0) 
    }
    static [uint32] SingleToUInt32Bits([single]$x) {
        return [BitConverter]::ToUInt32([BitConverter]::GetBytes($x), 0) 
    }
    static [int32] SingleToInt32Bits([single]$x) {
        return [BitConverter]::ToInt32([BitConverter]::GetBytes($x), 0) 
    }
    static [double] UInt64BitsToDouble([uint64]$x) {
        return [BitConverter]::ToDouble([BitConverter]::GetBytes($x), 0) 
    }
    static [single] UInt32BitsToSingle([uint32]$x) {
        return [BitConverter]::ToSingle([BitConverter]::GetBytes($x), 0) 
    }
}
class C1Math {
    static [bigint] RoundDecimal([bigint]$n, [int]$cut) {
        if ($cut -le 0) {
            return $n * [bigint]::Pow(10, - $cut)
        }
        $den = [bigint]::Pow(10, $cut)
        $q = [bigint]::Divide($n, $den)
        $rest = $n - $q * $den
        if (2 * $rest -gt $den -or (2 * $rest -eq $den -and !$q.IsEven)) {
            $q += [bigint]::One
        }
        return $q
    }
    static [string] FormatDecimal([double]$x, [string]$format, [Globalization.CultureInfo]$culture) {
        # Exact test reference for the three formats used by printf tests.
        if ($culture.Name -ne '' -or $format -cnotin @('F17', 'e16', 'g17')) {
            throw 'Unsupported decimal reference format'
        }
        if ([double]::IsNaN($x)) {
            return 'NaN'
        }
        if ($x -eq [double]::PositiveInfinity) {
            return 'Infinity'
        }
        if ($x -eq [double]::NegativeInfinity) {
            return '-Infinity'
        }
        $bits = [C1Bits]::DoubleToUInt64Bits($x)
        $sign = ''
        if (($bits -shr 63) -ne 0) {
            $sign = '-'
        }
        $field = [int](($bits -shr 52) -band [uint64]2047)
        $n = [bigint]($bits -band [Convert]::ToUInt64('fffffffffffff', 16))
        $ex = -1074
        if ($field -ne 0) {
            $n += [bigint]::One -shl 52
            $ex = $field - 1023 - 52
        }
        $scale = 0
        if ($ex -ge 0) {
            $n = $n -shl $ex
        }
        else {
            $scale = - $ex
            $n *= [bigint]::Pow(5, $scale)
        }
        if ($format -ceq 'F17') {
            $digits = [C1Math]::RoundDecimal($n, $scale - 17).ToString().PadLeft(18, [char]'0')
            return $sign + $digits.Insert($digits.Length - 17, '.')
        }
        $raw = $n.ToString()
        $decimalExponent = $raw.Length - $scale - 1
        if ($n.IsZero) {
            $decimalExponent = 0
        }
        $digits = [C1Math]::RoundDecimal($n, $raw.Length - 17).ToString().PadLeft(17, [char]'0')
        if ($digits.Length -gt 17) {
            $decimalExponent++
            $digits = $digits.Substring(0, 17)
        }
        $exSign = '+'
        if ($decimalExponent -lt 0) {
            $exSign = '-'
        }
        if ($format -ceq 'e16') {
            return $sign + $digits.Insert(1, '.') + 'e' + $exSign + ([Math]::Abs($decimalExponent)).ToString().PadLeft(3, [char]'0')
        }
        $digits = $digits.TrimEnd([char]'0')
        if (!$digits.Length) {
            $digits = '0'
        }
        if ($decimalExponent -lt -4 -or $decimalExponent -ge 17) {
            if ($digits.Length -gt 1) {
                $digits = $digits.Insert(1, '.')
            }
            return $sign + $digits + 'e' + $exSign + ([Math]::Abs($decimalExponent)).ToString().PadLeft(2, [char]'0')
        }
        $point = $decimalExponent + 1
        if ($point -le 0) {
            return $sign + '0.' + ('0' * (-$point)) + $digits
        }
        if ($point -ge $digits.Length) {
            return $sign + $digits + ('0' * ($point - $digits.Length))
        }
        return $sign + $digits.Insert($point, '.')
    }
    static [double] Parse([string]$text, [Globalization.CultureInfo]$culture) {
        if ($culture.Name -ne '') {
            throw 'Reference parser requires invariant culture'
        }
        return [C1Bits]::UInt64BitsToDouble([C1Bits]::ParseDecimalBits($text, 53))
    }
    static [double] Parse([string]$text, [Globalization.NumberStyles]$style, [Globalization.CultureInfo]$culture) {
        if ($style -ne [Globalization.NumberStyles]::Float) {
            throw 'Reference parser requires Float style'
        }
        return [C1Math]::Parse($text, $culture)
    }
    static [bigint] $PiHalf = 0
    static [double] Floor([double]$x) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Floor($x), $x) 
    }
    static [double] Ceiling([double]$x) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Ceiling($x), $x) 
    }
    static [double] Truncate([double]$x) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Truncate($x), $x) 
    }
    static [double] Round([double]$x) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Round($x), $x) 
    }
    static [double] Round([double]$x, [int]$digits) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Round($x, $digits), $x) 
    }
    static [double] Round([double]$x, [MidpointRounding]$mode) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Round($x, $mode), $x) 
    }
    static [double] Round([double]$x, [int]$digits, [MidpointRounding]$mode) {
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        return [C1Math]::CopySign([Math]::Round($x, $digits, $mode), $x) 
    }
    static [bigint] AtanReciprocal([int]$denominator) {
        $unit = [bigint]::One -shl 1400
        $term = [bigint]::Divide($unit, $denominator)
        $sum = $term
        $n = 3
        while (!$term.IsZero) {
            $term = - [bigint]::Divide($term, $denominator * $denominator)
            $sum += [bigint]::Divide($term, $n)
            $n += 2
        }
        return $sum
    }
    static [double] FixedDouble([bigint]$value) {
        $negative = $value.Sign -lt 0
        $value = [bigint]::Abs($value)
        if ($value.IsZero) {
            return 0.0
        }
        $exponent = [int][C1Bits]::GetBitLength($value) - 1 - 1400
        $cut = [Math]::Max(1400 + $exponent - 52, 326)
        $q = $value -shr $cut
        $rest = $value - ($q -shl $cut)
        $half = [bigint]::One -shl ($cut - 1)
        if ($rest -gt $half -or ($rest -eq $half -and !$q.IsEven)) {
            $q += [bigint]::One
        }
        if ($exponent -ge -1022) {
            $q += [bigint]($exponent + 1022) -shl 52
        }
        $bits = [uint64]$q
        if ($negative) {
            $bits = $bits -bor [Convert]::ToUInt64('8000000000000000', 16)
        }
        return [C1Bits]::UInt64BitsToDouble($bits)
    }
    static [double] Trig([double]$x, [int]$kind) {
        if ([double]::IsNaN($x) -or [double]::IsInfinity($x)) {
            return [double]::NaN
        }
        if ([Math]::Abs($x) -lt 1e-100) {
            if ($kind -eq 1) {
                return 1.0
            }
            else {
                return $x
            }
        }
        # Independent 1400-bit Machin pi and integer Taylor series: CLR 4's
        # native trig routines do not reliably reduce very large arguments.
        if ([C1Math]::PiHalf.IsZero) {
            [C1Math]::PiHalf = 8 * [C1Math]::AtanReciprocal(5) - 2 * [C1Math]::AtanReciprocal(239)
        }
        $unit = [bigint]::One -shl 1400
        $bits = [C1Bits]::DoubleToUInt64Bits([Math]::Abs($x))
        $ex = [int](($bits -shr 52) -band [uint64]2047) - 1023
        $mantissa = [bigint]($bits -band [Convert]::ToUInt64('fffffffffffff', 16)) + ([bigint]::One -shl 52)
        $value = $mantissa -shl ($ex - 52 + 1400)
        $n = [bigint]::Divide($value + ([C1Math]::PiHalf -shr 1), [C1Math]::PiHalf)
        $r = $value - $n * [C1Math]::PiHalf
        $quadrant = [int]($n % 4)
        $square = - [bigint]::Divide($r * $r, $unit)
        $sine = $r
        $term = $r
        for ($i = 3; $i -lt 1000; $i += 2) {
            $term = [bigint]::Divide($term * $square, $unit * $i * ($i - 1))
            $sine += $term
            if ($term.IsZero) {
                break
            }
        }
        $cosine = $unit
        $term = $unit
        for ($i = 2; $i -lt 1000; $i += 2) {
            $term = [bigint]::Divide($term * $square, $unit * $i * ($i - 1))
            $cosine += $term
            if ($term.IsZero) {
                break
            }
        }
        $sn = $sine
        $cs = $cosine
        switch ($quadrant) {
            1 {
                $sn = $cosine
                $cs = - $sine
            }2 {
                $sn = - $sine
                $cs = - $cosine
            }3 {
                $sn = - $cosine
                $cs = $sine
            }
        }
        if ($x -lt 0) {
            $sn = - $sn
        }
        if ($kind -eq 0) {
            return [C1Math]::FixedDouble($sn)
        }
        if ($kind -eq 1) {
            return [C1Math]::FixedDouble($cs)
        }
        return [C1Math]::FixedDouble([bigint]::Divide($sn * $unit, $cs))
    }
    static [double] Sin([double]$x) {
        return [C1Math]::Trig($x, 0) 
    }
    static [double] Cos([double]$x) {
        return [C1Math]::Trig($x, 1) 
    }
    static [double] Tan([double]$x) {
        return [C1Math]::Trig($x, 2) 
    }
    static [double] CopySign([double]$x, [double]$y) {
        $a = [BitConverter]::GetBytes($x)
        $b = [BitConverter]::GetBytes($y)
        $a[7] = ($a[7] -band 127) -bor ($b[7] -band 128)
        return [BitConverter]::ToDouble($a, 0)
    }
    static [double] BitIncrement([double]$x) {
        if ([double]::IsNaN($x) -or $x -eq [double]::PositiveInfinity) {
            return $x
        }
        if ($x -eq 0) {
            return [double]::Epsilon
        }
        $b = [C1Bits]::DoubleToUInt64Bits($x)
        if ($x -lt 0) {
            $b -= [uint64]1
        }
        else {
            $b += [uint64]1
        }
        return [C1Bits]::UInt64BitsToDouble($b)
    }
    static [double] BitDecrement([double]$x) {
        if ([double]::IsNaN($x) -or $x -eq [double]::NegativeInfinity) {
            return $x
        }
        if ($x -eq 0) {
            return - [double]::Epsilon
        }
        $b = [C1Bits]::DoubleToUInt64Bits($x)
        if ($x -lt 0) {
            $b += [uint64]1
        }
        else {
            $b -= [uint64]1
        }
        return [C1Bits]::UInt64BitsToDouble($b)
    }
    static [int] ILogB([double]$x) {
        if ($x -eq 0) {
            return [int]::MinValue
        }
        if ([double]::IsNaN($x) -or [double]::IsInfinity($x)) {
            return [int]::MaxValue
        }
        $b = [C1Bits]::DoubleToUInt64Bits($x)
        $ex = [int](($b -shr 52) -band [uint64]2047)
        if ($ex -ne 0) {
            return $ex - 1023
        }
        $b = $b -band [Convert]::ToUInt64('fffffffffffff', 16)
        $ex = -1074
        while ($b -gt 1) {
            $b = $b -shr 1
            $ex++
        }
        return $ex
    }
    static [double] ScaleB([double]$x, [int]$n) {
        # Intermediate scaling remains normal; only the final multiply may
        # round to a subnormal, avoiding a second underflow rounding.
        if ([double]::IsNaN($x)) {
            return $x * 1.0
        }
        if ($x -eq 0 -or [double]::IsInfinity($x)) {
            return $x
        }
        if ($n -gt 1023) {
            $x *= [C1Bits]::UInt64BitsToDouble([uint64]2046 -shl 52)
            $n -= 1023
            if ($n -gt 1023) {
                $x *= [C1Bits]::UInt64BitsToDouble([uint64]2046 -shl 52)
                $n -= 1023
                if ($n -gt 1023) {
                    $n = 1023
                }
            }
        }
        elseif ($n -lt -1022) {
            $x *= [C1Bits]::UInt64BitsToDouble([uint64]54 -shl 52)
            $n += 969
            if ($n -lt -1022) {
                $x *= [C1Bits]::UInt64BitsToDouble([uint64]54 -shl 52)
                $n += 969
                if ($n -lt -1022) {
                    $n = -1022
                }
            }
        }
        return $x * [C1Bits]::UInt64BitsToDouble([uint64]($n + 1023) -shl 52)
    }
    static [double] Log2([double]$x) {
        return [Math]::Log($x, 2.0) 
    }
}
# Independent single-precision test references evaluated in binary64, then
# rounded to binary32. Not a claim of bit-identical MathF transcendental APIs.
# Exact edge fixtures and arbitrary-precision references remain unchanged.
class C1MathF {
    static [single] Parse([string]$text, [Globalization.CultureInfo]$culture) {
        if ($culture.Name -ne '') {
            throw 'Reference parser requires invariant culture'
        }
        return [C1Bits]::UInt32BitsToSingle([uint32][C1Bits]::ParseDecimalBits($text, 24))
    }
    static [single] Parse([string]$text, [Globalization.NumberStyles]$style, [Globalization.CultureInfo]$culture) {
        if ($style -ne [Globalization.NumberStyles]::Float) {
            throw 'Reference parser requires Float style'
        }
        return [C1MathF]::Parse($text, $culture)
    }
    static [single] BitIncrement([single]$x) {
        if ([single]::IsNaN($x) -or $x -eq [single]::PositiveInfinity) {
            return $x
        }
        if ($x -eq 0) {
            return [single]::Epsilon
        }
        $b = [C1Bits]::SingleToUInt32Bits($x)
        if ($x -lt 0) {
            $b -= [uint32]1
        }
        else {
            $b += [uint32]1
        }
        return [C1Bits]::UInt32BitsToSingle($b)
    }
    static [single] BitDecrement([single]$x) {
        if ([single]::IsNaN($x) -or $x -eq [single]::NegativeInfinity) {
            return $x
        }
        if ($x -eq 0) {
            return - [single]::Epsilon
        }
        $b = [C1Bits]::SingleToUInt32Bits($x)
        if ($x -lt 0) {
            $b += [uint32]1
        }
        else {
            $b -= [uint32]1
        }
        return [C1Bits]::UInt32BitsToSingle($b)
    }
    static [single] Sin([single]$x) {
        return [single][C1Math]::Sin($x) 
    }
    static [single] Cos([single]$x) {
        return [single][C1Math]::Cos($x) 
    }
    static [single] Tan([single]$x) {
        return [single][C1Math]::Tan($x) 
    }
    static [single] Asin([single]$x) {
        return [single][Math]::Asin($x) 
    }
    static [single] Acos([single]$x) {
        return [single][Math]::Acos($x) 
    }
    static [single] Atan([single]$x) {
        return [single][Math]::Atan($x) 
    }
    static [single] Atan2([single]$y, [single]$x) {
        return [single][Math]::Atan2($y, $x) 
    }
    static [single] Sinh([single]$x) {
        return [single][Math]::Sinh($x) 
    }
    static [single] Cosh([single]$x) {
        return [single][Math]::Cosh($x) 
    }
    static [single] Tanh([single]$x) {
        return [single][Math]::Tanh($x) 
    }
    static [single] Exp([single]$x) {
        return [single][Math]::Exp($x) 
    }
    static [single] Log([single]$x) {
        return [single][Math]::Log($x) 
    }
    static [single] Log10([single]$x) {
        return [single][Math]::Log10($x) 
    }
    static [single] Log2([single]$x) {
        return [single][C1Math]::Log2($x) 
    }
    static [single] Pow([single]$x, [single]$y) {
        return [single][Math]::Pow($x, $y) 
    }
    static [single] Sqrt([single]$x) {
        return [single][Math]::Sqrt($x) 
    }
}
class C1ISOWeek {
    static [datetime] Thursday([datetime]$d) {
        $day = [int]$d.DayOfWeek
        if ($day -eq 0) {
            $day = 7
        }
        return $d.Date.AddDays(4 - $day)
    }
    static [int] GetYear([datetime]$d) {
        return [C1ISOWeek]::Thursday($d).Year 
    }
    static [int] GetWeekOfYear([datetime]$d) {
        return 1 + [int][Math]::Floor(([C1ISOWeek]::Thursday($d).DayOfYear - 1) / 7.0) 
    }
}
function ConvertTo-C1WindowsArguments([string[]]$Arguments) {
    $quoted = foreach ($argument in $Arguments) {
        # Windows CRT command-line quoting, including empty arguments and
        # backslashes immediately before quotes or the closing quote.
        '"' + [regex]::Replace([regex]::Replace($argument, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
    }
    return $quoted -join ' '
}
