$header = '#include "src/bootstrap-c99/base/lib/math-scale.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($variant in @(@('double', '', 'unsigned long long', 52, 1023), @('float', 'f', 'unsigned int', 23, 127), @('long double', 'l', 'unsigned long long', 52, 1023))) {
    $t = $variant[0]
    $s = $variant[1]
    $u = $variant[2]
    $p = $variant[3]
    $bias = $variant[4]
    $lead = [UInt64]1 -shl $p
    $inf = [UInt64]($bias * 2 + 1) -shl $p
    $sign = if ($p -eq 23) {
        [UInt64]2147483648
    }
    else {
        [UInt64]::Parse('8000000000000000', [Globalization.NumberStyles]::HexNumber)
    }
    $one = [UInt64]$bias -shl $p
    $eight = [UInt64]($bias + 3) -shl $p
    foreach ($stem in 'ldexp', 'scalbn', 'scalbln') {
        $fn = $stem + $s
        $code = "int check($u input,int n,$u expected,int flags,int err){union{$t f;$u u;}a,b;errno=123;feclearexcept(FE_ALL_EXCEPT);a.u=input;b.f=$fn(a.f,n);if(b.u!=expected)return 1;if(fetestexcept(FE_ALL_EXCEPT)!=flags)return 2;if(errno!=(err?ERANGE:123))return 3;return 0;}int main(void){fenv_t saved;int mode;fegetenv(&saved);"
        for ($mode = 0; $mode -lt 4; $mode++) {
            $code += "fesetround($($mode -shl 13));"
            $up = $mode -eq 2
            $down = $mode -eq 1
            $nearest = $mode -eq 0
            $cases = @(
                @($one, 3, $eight, 0, 0),
                @([UInt64]1, 0, [UInt64]1, 0, 0),
                @([UInt64]1, 1, [UInt64]2, 0, 0),
                @($lead, -1, ($lead -shr 1), 0, 0),
                @([UInt64]1, -1, [UInt64]([int]$up), 48, 1),
                @([UInt64]3, -1, [UInt64]($(if ($up -or $nearest) {
                                2
                            }
                            else {
                                1
                            })), 48, 1),
                @(($inf - 1), 1, $(if ($up -or $nearest) {
                            $inf
                        }
                        else {
                            $inf - 1
                        }), 40, 1),
                @(($sign -bor ($inf - 1)), 1, ($sign -bor $(if ($down -or $nearest) {
                                $inf
                            }
                            else {
                                $inf - 1
                            })), 40, 1),
                @(($lead * 2 - 1), -1, $(if ($up -or $nearest) {
                            $lead
                        }
                        else {
                            $lead - 1
                        }), $(if ($up -or $nearest) {
                            32
                        }
                        else {
                            48
                        }), $(if ($up -or $nearest) {
                            0
                        }
                        else {
                            1
                        })),
                @($one, (-2147483647 - 1), [UInt64]([int]$up), 48, 1),
                @($one, 2147483647, $(if ($up -or $nearest) {
                            $inf
                        }
                        else {
                            $inf - 1
                        }), 40, 1),
                @(($sign -bor [UInt64]1), -1, ($sign -bor [UInt64]([int]$down)), 48, 1),
                @($sign, (-2147483647 - 1), $sign, 0, 0),
                @($inf, (-2147483647 - 1), $inf, 0, 0),
                @(($inf -bor [UInt64]123), 2147483647, ($inf -bor [UInt64]123), 0, 0)
            )
            $index = 0
            foreach ($case in $cases) {
                $index++
                $input = [UInt64]$case[0]
                $expected = [UInt64]$case[2]
                $n = if ($case[1] -eq -2147483648) {
                    '(-2147483647-1)'
                }
                else {
                    [string]$case[1]
                }
                $code += 'if(check(0x' + $input.ToString('X') + 'ULL,' + $n + ',0x' + $expected.ToString('X') + 'ULL,' + $case[3] + ',' + $case[4] + '))return ' + $index + ';'
            }
        }
        $code += 'fesetenv(&saved);return 0;}'
        MathCheck "math-scale-$fn" ($header + $code)
    }
}
