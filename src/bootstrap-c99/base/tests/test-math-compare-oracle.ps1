$random = [Random]::new(724968)
$prefix = '#include "src/bootstrap-c99/base/lib/math-classify.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
$types = @(@('float', 'unsigned int'), @('double', 'unsigned long long'), @('long double', 'unsigned long long'))
foreach ($left in $types) {
    foreach ($right in $types) {
        for ($batch = 0; $batch -lt 4; $batch++) {
            $code = "int main(void){union{$($left[0]) f;$($left[1]) u;}a;union{$($right[0]) f;$($right[1]) u;}b;fenv_t saved;feholdexcept(&saved);"
            for ($i = 0; $i -lt 16; $i++) {
                $values = @()
                $bits = @()
                foreach ($type in @($left[0], $right[0])) {
                    $single = $type -eq 'float'
                    $bytes = [byte[]]::new($(if ($single) {
                                4
                            }
                            else {
                                8
                            }))
                    $random.NextBytes($bytes)
                    $values += , $(if ($single) {
                            [double][BitConverter]::ToSingle($bytes, 0)
                        }
                        else {
                            [BitConverter]::ToDouble($bytes, 0)
                        })
                    $bits += , $(if ($single) {
                            [BitConverter]::ToUInt32($bytes, 0)
                        }
                        else {
                            [BitConverter]::ToUInt64($bytes, 0)
                        })
                }
                $a = [double]$values[0]
                $b = [double]$values[1]
                $unordered = [double]::IsNaN($a) -or [double]::IsNaN($b)
                $expected = @{
                    'isless' = (-not $unordered -and $a -lt $b)
                    'islessequal' = (-not $unordered -and $a -le $b)
                    'isgreater' = (-not $unordered -and $a -gt $b)
                    'isgreaterequal' = (-not $unordered -and $a -ge $b)
                    'islessgreater' = (-not $unordered -and $a -ne $b)
                    'isunordered' = $unordered
                }
                $code += 'a.u=0x' + $bits[0].ToString('X') + 'ULL;b.u=0x' + $bits[1].ToString('X') + 'ULL;'
                foreach ($name in $expected.Keys) {
                    $code += "if(!!$name(a.f,b.f)!=$([int]$expected[$name]))return " + ($i + 1) + ';'
                }
            }
            $code += 'if(fetestexcept(FE_ALL_EXCEPT))return 90;fesetenv(&saved);return 0;}'
            MathCheck "math-compare-$($left[0])-$($right[0])-$batch" ($prefix + $code)
        }
    }
}
