
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($variant in @(@('double', '', 'unsigned long long', 52, 1023), @('float', 'f', 'unsigned int', 23, 127), @('long double', 'l', 'unsigned long long', 52, 1023))) {
    $t = $variant[0]
    $suffix = $variant[1]
    $word = $variant[2]
    $p = [int]$variant[3]
    $bias = [int]$variant[4]
    $sign = if ($p -eq 23) {
        [uint64]2147483648
    }
    else {
        [uint64]::Parse('8000000000000000', [Globalization.NumberStyles]::HexNumber)
    }
    $inf = [uint64](2 * $bias + 1) -shl $p
    $one = [uint64]$bias -shl $p
    $tinyNormal = [uint64]1 -shl $p
    $cases = @(
        @([uint64]0, $sign, $sign, 0, 0),
        @($sign, [uint64]0, [uint64]0, 0, 0),
        @([uint64]0, $one, [uint64]1, 34, 48),
        @($sign, ($one-bor $sign), ($sign-bor [uint64]1), 34, 48),
        @([uint64]1, [uint64]0, [uint64]0, 34, 48),
        @(($sign-bor [uint64]1), [uint64]0, $sign, 34, 48),
        @($tinyNormal, [uint64]0, ($tinyNormal - 1), 34, 48),
        @(($tinyNormal - 1), $one, $tinyNormal, 0, 0),
        @(($inf - 1), $inf, $inf, 34, 40),
        @(($sign-bor ($inf - 1)), ($sign-bor $inf), ($sign-bor $inf), 34, 40),
        @($inf, [uint64]0, ($inf - 1), 0, 0),
        @(($sign-bor $inf), [uint64]0, ($sign-bor ($inf - 1)), 0, 0),
        @($one, $inf, ($one + 1), 0, 0),
        @($one, [uint64]0, ($one - 1), 0, 0)
    )
    foreach ($base in @('nextafter', 'nexttoward')) {
        $fn = $base + $suffix
        $src = $prefix + "int main(void){union{$t f;$word u;}a,b,c;fenv_t saved,mode;int r;fegetenv(&saved);"
        $src += 'for(r=0;r<4;r++){mode.__mxcsr=0x9fc0U|(r<<13);'
        $n = 0
        foreach ($case in $cases) {
            $n++
            $src += 'fesetenv(&mode);errno=0;a.u=0x' + ([uint64]$case[0]).ToString('X') + 'ULL;b.u=0x' + ([uint64]$case[1]).ToString('X') + 'ULL;'
            $src += "c.f=$fn(a.f,b.f);" + 'if(c.u!=0x' + ([uint64]$case[2]).ToString('X') + 'ULL||errno!=' + $case[3] + '||fetestexcept(FE_ALL_EXCEPT)!=' + $case[4] + ')return ' + $n + ';'
        }
        $src += '}fesetenv(&saved);return 0;}'
        ExtendedCheck "adjacent-$fn-boundaries" $src
    }
}
ExtendedCheck 'adjacent-wide-direction' ($prefix + @'
int main(void){
 union{float f;unsigned int u;}a;
 fenv_t saved;feholdexcept(&saved);
 a.f=nexttowardf(1.0f,0x1.0000000000001p0L);if(a.u!=0x3f800001U)return 1;
 if(fetestexcept(FE_ALL_EXCEPT))return 4;
 a.f=nextafterf(1.0f,0x1.0000000000001p0L);if(a.u!=0x3f800000U)return 2;
 /* Discard the inexact argument conversion to float before the call. */
 feclearexcept(FE_ALL_EXCEPT);
 a.f=nexttowardf(1.0f,0x1.fffffffffffffp-1L);if(a.u!=0x3f7fffffU)return 3;
 if(fetestexcept(FE_ALL_EXCEPT))return 4;
 fesetenv(&saved);return 0;
}
'@)
ExtendedCheck 'adjacent-nan-status' ($prefix + @'
int main(void){
 union{float f;unsigned int u;}a,c;
 union{long double f;unsigned long long u;}b;
 fenv_t saved;feholdexcept(&saved);errno=123;
 a.u=0x7fc01234U;b.u=0x3ff0000000000000ULL;
 c.f=nexttowardf(a.f,b.f);if(c.u!=a.u||fetestexcept(FE_ALL_EXCEPT))return 1;
 a.u=0x3f800000U;b.u=0xfff0000000000001ULL;
 c.f=nexttowardf(a.f,b.f);if(c.u!=0xffc00000U||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 2;
 feclearexcept(FE_ALL_EXCEPT);a.u=0x7f800001U;b.u=0x7ff8000000000000ULL;
 c.f=nexttowardf(a.f,b.f);if(c.u!=0x7fc00001U||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 3;
 if(errno!=123)return 4;fesetenv(&saved);return 0;
}
'@)
# Independent .NET neighboring-value oracle. It only supplies source literals.
$random = [Random]::new(133192)
foreach ($variant in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $t = $variant[0]
    $suffix = $variant[1]
    $word = $variant[2]
    $single = $suffix -eq 'f'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $src = $prefix + "int main(void){union{$t f;$word u;}a,b;fenv_t saved;feholdexcept(&saved);"
        for ($i = 0; $i -lt 32; $i++) {
            do {
                $bytes = [byte[]]::new($(if ($single) {
                            4
                        }
                        else {
                            8
                        }))
                $random.NextBytes($bytes)
                $v = if ($single) {
                    [double][BitConverter]::ToSingle($bytes, 0)
                }
                else {
                    [BitConverter]::ToDouble($bytes, 0)
                }
            }while ([double]::IsNaN($v) -or [double]::IsInfinity($v))
            $up = $i % 2 -eq 0
            $expected = if ($single) {
                if ($up) {
                    [C1MathF]::BitIncrement([single]$v)
                }
                else {
                    [C1MathF]::BitDecrement([single]$v)
                }
            }
            else {
                if ($up) {
                    [C1Math]::BitIncrement($v)
                }
                else {
                    [C1Math]::BitDecrement($v)
                }
            }
            $bits = if ($single) {
                [BitConverter]::ToUInt32($bytes, 0)
            }
            else {
                [BitConverter]::ToUInt64($bytes, 0)
            }
            $outbits = if ($single) {
                [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$expected), 0)
            }
            else {
                [BitConverter]::ToUInt64([BitConverter]::GetBytes([double]$expected), 0)
            }
            $dir = if ($up) {
                'HUGE_VALL'
            }
            else {
                '-HUGE_VALL'
            }
            $src += 'a.u=0x' + $bits.ToString('X') + 'ULL;'
            foreach ($fn in @('nextafter', 'nexttoward')) {
                $src += "b.f=$fn$suffix(a.f,$dir);" + 'if(b.u!=0x' + $outbits.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
            }
        }
        $src += 'fesetenv(&saved);return 0;}'
        ExtendedCheck "adjacent-oracle-$suffix-$batch" $src
    }
}
