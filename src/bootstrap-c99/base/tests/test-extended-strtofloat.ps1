. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($v in @(@('double', 'strtod', 'unsigned long long'), @('float', 'strtof', 'unsigned int'), @('long double', 'strtold', 'unsigned long long'))) {
    $t = $v[0]
    $fn = $v[1]
    $word = $v[2]
    $single = $fn -eq 'strtof'
    $cases = @(
        @('', 0, '0', '0'), @('  +x', 0, '0', '0'), @('.', 0, '0', '0'), @('0x.', 1, '0', '0'),
        @(' -0tail', 3, '8000000000000000', '80000000'), @('12tail', 2, '4028000000000000', '41400000'),
        @('  +.5!', 5, '3FE0000000000000', '3F000000'), @('1e+', 1, '3FF0000000000000', '3F800000'),
        @('1e-2end', 4, '3F847AE147AE147B', '3C23D70A'), @('0x1.8p+2z', 8, '4018000000000000', '40C00000'),
        @('0X.8p1?', 6, '3FF0000000000000', '3F800000'), @('0x12tail', 4, '4032000000000000', '41900000'),
        @('INF!', 3, '7FF0000000000000', '7F800000'), @('-InFiNiTy!', 9, 'FFF0000000000000', 'FF800000'),
        @('infinite', 3, '7FF0000000000000', '7F800000'), @('NAN(payload_123)z', 16, '7FF8000000000000', '7FC00000'),
        @('-nan()x', 6, 'FFF8000000000000', 'FFC00000'), @('nan(bad!)', 3, '7FF8000000000000', '7FC00000'),
        @('0e999999999999999', 17, '0', '0'), @('1.0f', 3, '3FF0000000000000', '3F800000')
    )
    $src = $prefix + "int main(void){union{$t f;$word u;}a;char *s,*end;int n;"
    $n = 0
    foreach ($c in $cases) {
        $n++
        $hex = $c[$(if ($single) {
                3
            }
            else {
                2
            })]
        $src += 's="' + $c[0] + '";end=(char *)0;' + "a.f=$fn(s,&end);" + 'if(a.u!=0x' + $hex + 'ULL||end-s!=' + $c[1] + ')return ' + $n + ';'
    }
    $src += 'return 0;}'
    ExtendedCheck "strtofloat-$fn-syntax" $src
}
ExtendedCheck 'strtofloat-rounding-range' ($prefix + @'
int main(void){
 union{double f;unsigned long long u;}a;
 union{float f;unsigned int u;}b;
 fenv_t saved,mode;int r;
 fegetenv(&saved);
 for(r=0;r<4;r++){
  mode.__mxcsr=0x9fc0U|(r<<13);fesetenv(&mode);errno=123;
  a.f=strtod("0x1p-1074",0);
  if(a.u!=1||errno!=123||fetestexcept(FE_ALL_EXCEPT))return 1;
  a.f=strtod("1e-10000",0);
  if(a.u!=(r==2?1ULL:0ULL)||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_UNDERFLOW|FE_INEXACT))return 2;
  fesetenv(&mode);errno=123;a.f=strtod("-1e-10000",0);
  if(a.u!=(0x8000000000000000ULL|(r==1?1ULL:0ULL))||errno!=ERANGE)return 3;
  fesetenv(&mode);errno=123;a.f=strtod("1e10000",0);
  if(a.u!=((r==0||r==2)?0x7ff0000000000000ULL:0x7fefffffffffffffULL)||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=(FE_OVERFLOW|FE_INEXACT))return 4;
  fesetenv(&mode);errno=123;a.f=strtod("-1e10000",0);
  if(a.u!=((r==0||r==1)?0xfff0000000000000ULL:0xffefffffffffffffULL)||errno!=ERANGE)return 5;
  fesetenv(&mode);errno=123;b.f=strtof("1.000000059604644775390625",0);
  if(b.u!=(r==2?0x3f800001U:0x3f800000U)||errno!=123||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 6;
  fesetenv(&mode);errno=123;a.f=strtod("0.1",0);
  if(a.u!=((r==1||r==3)?0x3fb9999999999999ULL:0x3fb999999999999aULL)||errno!=123)return 7;
 }
 fesetenv(&saved);return 0;
}
'@)
ExtendedCheck 'strtofloat-long-sticky' ($prefix + @'
int main(void){
 char text[6000],*end;int i;
 union{float f;unsigned int u;}a;
 union{double f;unsigned long long u;}b;
 fenv_t saved;feholdexcept(&saved);fesetround(FE_TONEAREST);
 strcpy(text,"1.000000059604644775390625");
 for(i=26;i<5000;i++)text[i]='0';
 text[5000]='1';text[5001]=0;
 a.f=strtof(text,&end);if(a.u!=0x3f800001U||end!=text+5001)return 1;
 text[5000]='0';a.f=strtof(text,0);if(a.u!=0x3f800000U)return 2;
 text[0]='1';for(i=1;i<5001;i++)text[i]='0';strcpy(text+5001,"e-5000");
 b.f=strtod(text,0);if(b.u!=0x3ff0000000000000ULL)return 3;
 if(atof("12.5")!=12.5)return 4;
 fesetenv(&saved);return 0;
}
'@)
$random = [Random]::new(98502)
foreach ($v in @(@('double', 'strtod', 'unsigned long long'), @('float', 'strtof', 'unsigned int'), @('long double', 'strtold', 'unsigned long long'))) {
    $t = $v[0]
    $fn = $v[1]
    $word = $v[2]
    $single = $fn -eq 'strtof'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $src = $prefix + "int main(void){union{$t f;$word u;}a;char *s,*end;fenv_t saved;feholdexcept(&saved);fesetround(FE_TONEAREST);"
        for ($i = 0; $i -lt 32; $i++) {
            $n = $random.Next(1, 181)
            $s = ''
            if ($random.Next(2)) {
                $s = '-'
            }
            $s += [string]$random.Next(1, 10)
            $s += '.'
            for ($j = 1; $j -lt $n; $j++) {
                $s += [string]$random.Next(10)
            }
            $s += 'e' + [string]$random.Next(-450, 451)
            $x = if ($single) {
                [C1MathF]::Parse($s, [Globalization.CultureInfo]::InvariantCulture)
            }
            else {
                [C1Math]::Parse($s, [Globalization.CultureInfo]::InvariantCulture)
            }
            $bits = if ($single) {
                [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$x), 0)
            }
            else {
                [BitConverter]::ToUInt64([BitConverter]::GetBytes([double]$x), 0)
            }
            $src += 's="' + $s + '";' + "a.f=$fn(s,&end);" + 'if(a.u!=0x' + $bits.ToString('X') + 'ULL||*end)return ' + ($i + 1) + ';'
        }
        $src += 'fesetenv(&saved);return 0;}'
        ExtendedCheck "strtofloat-$fn-decimal-$batch" $src
    }
}
function ExactDecimal([Numerics.BigInteger]$Coefficient, [int]$BinaryExponent, [int]$Delta) {
    $scale = 0
    $n = $Coefficient
    if ($BinaryExponent -ge 0) {
        $n = $n -shl $BinaryExponent
    }
    else {
        $scale = - $BinaryExponent
        $n *= [Numerics.BigInteger]::Pow(5, $scale)
    }
    # Values immediately below/above the midpoint beyond the retained prefix.
    if ($Delta) {
        $scale += 1600
        $n = $n * [Numerics.BigInteger]::Pow(10, 1600) + $Delta
    }
    $s = $n.ToString([Globalization.CultureInfo]::InvariantCulture)
    if ($scale -eq 0) {
        return $s
    }
    if ($s.Length -le $scale) {
        return '0.' + ('0' * ($scale - $s.Length)) + $s
    }
    return $s.Insert($s.Length - $scale, '.')
}
foreach ($v in @(@('double', 'strtod', 'unsigned long long', 53, 1023), @('float', 'strtof', 'unsigned int', 24, 127), @('long double', 'strtold', 'unsigned long long', 53, 1023))) {
    $t = $v[0]
    $fn = $v[1]
    $word = $v[2]
    $p = [int]$v[3]
    $bias = [int]$v[4]
    $lead = [uint64]1 -shl ($p - 1)
    $sign = if ($p -eq 24) {
        [uint64]2147483648
    }
    else {
        [uint64]::Parse('8000000000000000', [Globalization.NumberStyles]::HexNumber)
    }
    $inf = [uint64](2 * $bias + 1) -shl ($p - 1)
    $boundaries = @(
        @([uint64]0, (1 - $bias), [uint64]0),
        @(($lead - 1), (1 - $bias), ($lead - 1)),
        @($lead, 0, ([uint64]$bias -shl ($p - 1))),
        @(($lead * 2 - 1), $bias, ($inf - 1))
    )
    for ($mode = 0; $mode -lt 4; $mode++) {
        $src = $prefix + "int main(void){union{$t f;$word u;}a;fenv_t saved,mode;fegetenv(&saved);"
        $src += 'mode.__mxcsr=0x9fc0U|' + ($mode -shl 13) + ';'
        $index = 0
        foreach ($b in $boundaries) {
            foreach ($delta in @(-1, 0, 1)) {
                foreach ($negative in @($false, $true)) {
                    $index++
                    $m = [uint64]$b[0]
                    $low = [uint64]$b[2]
                    $number = ExactDecimal ([Numerics.BigInteger]$m * 2 + 1) ([int]$b[1] - $p) $delta
                    if ($negative) {
                        $number = '-' + $number
                    }
                    $increment = if ($mode -eq 0) {
                        $delta -gt 0 -or ($delta -eq 0 -and ($m-band 1))
                    }
                    else {
                        ($mode -eq 1 -and $negative) -or ($mode -eq 2 -and -not $negative)
                    }
                    $expected = $low + $(if ($increment) {
                            [uint64]1
                        }
                        else {
                            [uint64]0
                        })
                    $flags = if ($expected -eq $inf) {
                        40
                    }
                    elseif ($expected -lt $lead) {
                        48
                    }
                    else {
                        32
                    }
                    $err = if ($flags -eq 32) {
                        123
                    }
                    else {
                        34
                    }
                    if ($negative) {
                        $expected = $expected-bor $sign
                    }
                    $src += 'fesetenv(&mode);errno=123;' + "a.f=$fn(" + '"' + $number + '",0);if(a.u!=0x' + $expected.ToString('X') + 'ULL||errno!=' + $err + '||fetestexcept(FE_ALL_EXCEPT)!=' + $flags + ')return ' + $index + ';'
                }
            }
        }
        $src += 'fesetenv(&saved);return 0;}'
        ExtendedCheck "strtofloat-$fn-midpoints-$mode" $src
    }
}
