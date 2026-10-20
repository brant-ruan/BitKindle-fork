
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($variant in @(@('double', '', 'unsigned long long', 52, 1023, -1074), @('float', 'f', 'unsigned int', 23, 127, -149), @('long double', 'l', 'unsigned long long', 52, 1023, -1074))) {
    $t = $variant[0]
    $suffix = $variant[1]
    $word = $variant[2]
    $p = [int]$variant[3]
    $bias = [int]$variant[4]
    $minex = $variant[5]
    $sign = if ($p -eq 23) {
        [uint64]2147483648
    }
    else {
        [uint64]::Parse('8000000000000000', [Globalization.NumberStyles]::HexNumber)
    }
    $inf = [uint64](2 * $bias + 1) -shl $p
    $one = [uint64]$bias -shl $p
    $quiet = [uint64]1 -shl ($p - 1)
    $src = $prefix + "int main(void){union{$t f;$word u;}a,b,c;fenv_t saved,mode;int r;fegetenv(&saved);"
    $src += 'for(r=0;r<4;r++){mode.__mxcsr=0x9fc0U|(r<<13)|0x24U;fesetenv(&mode);errno=123;'
    $pairs = @(
        @([uint64]0, $sign, [uint64]0, $sign),
        @($sign, [uint64]0, [uint64]0, $sign),
        @([uint64]1, [uint64]2, [uint64]2, [uint64]1),
        @(($sign-bor [uint64]1), ($sign-bor [uint64]2), ($sign-bor [uint64]1), ($sign-bor [uint64]2)),
        @($inf, ($sign-bor $inf), $inf, ($sign-bor $inf)),
        @(($inf-bor $quiet), $one, $one, $one),
        @($one, ($inf-bor $quiet), $one, $one),
        @(($inf-bor $quiet-bor [uint64]1), ($inf-bor $quiet-bor [uint64]2), ($inf-bor $quiet-bor [uint64]1), ($inf-bor $quiet-bor [uint64]1))
    )
    $i = 0
    foreach ($v in $pairs) {
        $i++
        $src += 'a.u=0x' + ([uint64]$v[0]).ToString('X') + 'ULL;b.u=0x' + ([uint64]$v[1]).ToString('X') + 'ULL;'
        $src += "c.f=fmax$suffix(a.f,b.f);" + 'if(c.u!=0x' + ([uint64]$v[2]).ToString('X') + 'ULL)return ' + $i + ';'
        $src += "c.f=fmin$suffix(a.f,b.f);" + 'if(c.u!=0x' + ([uint64]$v[3]).ToString('X') + 'ULL)return ' + ($i + 10) + ';'
    }
    $src += 'if(fetestexcept(FE_ALL_EXCEPT)!=0x24||errno!=123)return 30;'
    $src += 'a.u=0x' + ($inf-bor [uint64]1).ToString('X') + 'ULL;b.u=0x' + $one.ToString('X') + 'ULL;'
    $src += "c.f=fmax$suffix(a.f,b.f);" + 'if(c.u!=b.u||fetestexcept(FE_ALL_EXCEPT)!=0x25||errno!=123)return 31;'
    $src += '}fesetenv(&saved);return 0;}'
    ExtendedCheck "math-minmax-$suffix-special" $src
    $src = $prefix + "int main(void){union{$t f;$word u;}a,b;fenv_t saved,mode;int r;fegetenv(&saved);"
    $src += 'for(r=0;r<4;r++){mode.__mxcsr=0x9fc0U|(r<<13);fesetenv(&mode);errno=123;'
    $src += "a.u=1;if(ilogb$suffix(a.f)!=$minex||logb$suffix(a.f)!=$minex)return 1;"
    $src += "if(ilogb$suffix(-3.0)!=1||logb$suffix(-3.0)!=1||ilogb$suffix(0.75)!=-1||logb$suffix(0.75)!=-1)return 2;"
    $src += 'if(errno!=123||fetestexcept(FE_ALL_EXCEPT))return 3;'
    $src += "if(ilogb$suffix(0.0)!=FP_ILOGB0||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 4;"
    $src += 'feclearexcept(FE_ALL_EXCEPT);errno=123;'
    $src += "b.f=logb$suffix(-0.0);" + 'if(b.u!=0x' + ($sign-bor $inf).ToString('X') + 'ULL||errno!=ERANGE||fetestexcept(FE_ALL_EXCEPT)!=FE_DIVBYZERO)return 5;'
    $src += 'feclearexcept(FE_ALL_EXCEPT);errno=123;a.u=0x' + ($sign-bor $inf).ToString('X') + 'ULL;'
    $src += "b.f=logb$suffix(a.f);" + 'if(b.u!=0x' + $inf.ToString('X') + 'ULL||errno!=123||fetestexcept(FE_ALL_EXCEPT))return 6;'
    $src += "if(ilogb$suffix(a.f)!=2147483647||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 7;"
    $src += 'feclearexcept(FE_ALL_EXCEPT);errno=123;a.u=0x' + ($inf-bor $quiet).ToString('X') + 'ULL;'
    $src += "b.f=logb$suffix(a.f);" + 'if(b.u!=a.u||errno!=123||fetestexcept(FE_ALL_EXCEPT))return 8;'
    $src += "if(ilogb$suffix(a.f)!=FP_ILOGBNAN||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 9;"
    $src += '}fesetenv(&saved);return 0;}'
    ExtendedCheck "math-logb-$suffix-special" $src
}
# Reference expectations for finite values, independent from compiler helpers.
$rng = [Random]::new(345079)
foreach ($v in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $t = $v[0]
    $s = $v[1]
    $u = $v[2]
    $single = $s -eq 'f'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $src = $prefix + "int main(void){union{$t f;$u u;}a,b,c;fenv_t saved;feholdexcept(&saved);"
        for ($i = 0; $i -lt 32; $i++) {
            $nums = @()
            $raw = @()
            for ($j = 0; $j -lt 2; $j++) {
                do {
                    $bytes = [byte[]]::new($(if ($single) {
                                4
                            }
                            else {
                                8
                            }))
                    $rng.NextBytes($bytes)
                    $x = if ($single) {
                        [double][BitConverter]::ToSingle($bytes, 0)
                    }
                    else {
                        [BitConverter]::ToDouble($bytes, 0)
                    }
                }while ([double]::IsNaN($x) -or [double]::IsInfinity($x) -or $x -eq 0)
                $nums += , $x
                $raw += , $(if ($single) {
                        [BitConverter]::ToUInt32($bytes, 0)
                    }
                    else {
                        [BitConverter]::ToUInt64($bytes, 0)
                    })
            }
            $hi = if ($nums[0] -gt $nums[1]) {
                $raw[0]
            }
            else {
                $raw[1]
            }
            $lo = if ($nums[0] -lt $nums[1]) {
                $raw[0]
            }
            else {
                $raw[1]
            }
            $ex = [C1Math]::ILogB($nums[0])
            $src += 'a.u=0x' + $raw[0].ToString('X') + 'ULL;b.u=0x' + $raw[1].ToString('X') + 'ULL;'
            $src += "c.f=fmax$s(a.f,b.f);" + 'if(c.u!=0x' + $hi.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
            $src += "c.f=fmin$s(a.f,b.f);" + 'if(c.u!=0x' + $lo.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
            $src += "if(ilogb$s(a.f)!=$ex||logb$s(a.f)!=$ex)return " + ($i + 1) + ';'
        }
        $src += 'if(fetestexcept(FE_ALL_EXCEPT))return 100;fesetenv(&saved);return 0;}'
        ExtendedCheck "math-order-exponent-$s-$batch" $src
    }
}
