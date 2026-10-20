
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$random = [Random]::new(903184)
$prefix = '#include "src/bootstrap-c99/base/lib/math-scale.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($variant in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $t = $variant[0]
    $s = $variant[1]
    $u = $variant[2]
    $single = $s -eq 'f'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $code = "int main(void){union{$t f;$u u;}a,b;fesetround(FE_TONEAREST);"
        for ($i = 0; $i -lt 32; $i++) {
            do {
                $bytes = [byte[]]::new($(if ($single) {
                            4
                        }
                        else {
                            8
                        }))
                $random.NextBytes($bytes)
                $value = if ($single) {
                    [double][BitConverter]::ToSingle($bytes, 0)
                }
                else {
                    [BitConverter]::ToDouble($bytes, 0)
                }
            }while ([double]::IsNaN($value) -or [double]::IsInfinity($value))
            $n = $random.Next(-2200, 2201)
            $expected = [C1Math]::ScaleB($value, $n)
            if ($single) {
                $inputBits = [BitConverter]::ToUInt32($bytes, 0)
                $expectedBits = [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$expected), 0)
            }
            else {
                $inputBits = [BitConverter]::ToUInt64($bytes, 0)
                $expectedBits = [BitConverter]::ToUInt64([BitConverter]::GetBytes($expected), 0)
            }
            $code += 'a.u=0x' + $inputBits.ToString('X') + 'ULL;'
            foreach ($fn in 'ldexp', 'scalbn', 'scalbln') {
                $code += "b.f=$fn$s(a.f,$n);if(b.u!=0x" + $expectedBits.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
            }
        }
        $code += 'return 0;}'
        MathCheck "math-scale-oracle-$s-$batch" ($prefix + $code)
    }
}
