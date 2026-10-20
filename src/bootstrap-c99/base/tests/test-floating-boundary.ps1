# .NET is used only as a test oracle. The generated text is C test input.
# Every test executable is generated exclusively by the custom compiler.
$random = [Random]::new(739121)
foreach ($direction in 'widen', 'narrow') {
    for ($batch = 0; $batch -lt 16; $batch++) {
        $source = 'int main(void){union{float f;unsigned u;}a;union{double d;unsigned long long u;}b;'
        for ($i = 0; $i -lt 32; $i++) {
            if ($direction -eq 'widen') {
                do {
                    $bytes = [byte[]]::new(4)
                    $random.NextBytes($bytes)
                    $bits = [BitConverter]::ToUInt32($bytes, 0)
                } while (($bits -band 0x7f800000) -eq 0x7f800000)
                $value = [BitConverter]::ToSingle($bytes, 0)
                $expected = [BitConverter]::ToUInt64([BitConverter]::GetBytes([double]$value), 0)
                $source += 'a.u=0x' + $bits.ToString('X') + 'U;b.d=a.f;if(b.u!=0x' + $expected.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
            }
            else {
                do {
                    $bytes = [byte[]]::new(8)
                    $random.NextBytes($bytes)
                    $value = [BitConverter]::ToDouble($bytes, 0)
                } while ([double]::IsNaN($value) -or [double]::IsInfinity($value) -or [Math]::Abs($value) -gt [double][single]::MaxValue)
                $bits = [BitConverter]::ToUInt64($bytes, 0)
                $expected = [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$value), 0)
                $source += 'b.u=0x' + $bits.ToString('X') + 'ULL;a.f=b.d;if(a.u!=0x' + $expected.ToString('X') + 'U)return ' + ($i + 1) + ';'
            }
        }
        $source += 'return 0;}'
        FloatingCheck "fp-boundary-$direction-$batch" $source
    }
}
