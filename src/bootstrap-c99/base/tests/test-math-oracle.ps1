
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
# External arithmetic only computes test expectations. All EXEs use custom C1.
function MathExpectedBits([double]$Value, [bool]$Single) {
    if ($Single) {
        return '0x' + [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$Value), 0).ToString('X') + 'U'
    }
    return '0x' + [BitConverter]::ToUInt64([BitConverter]::GetBytes($Value), 0).ToString('X') + 'ULL'
}
$random = [Random]::new(440613)
$prefix = '#include "src/bootstrap-c99/base/lib/math-bits.c"' + [char]10
foreach ($variant in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $type = $variant[0]
    $suffix = $variant[1]
    $word = $variant[2]
    $single = $suffix -eq 'f'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $code = "int main(void){union{$type f;$word u;}a,b,c;int e;"
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
            $inputBits = MathExpectedBits $value $single
            $code += "a.u=$inputBits;"
            foreach ($operation in 'floor', 'ceil', 'trunc', 'round', 'fabs') {
                $expected = switch ($operation) {
                    'floor' {
                        [C1Math]::Floor($value)
                    }
                    'ceil' {
                        [C1Math]::Ceiling($value)
                    }
                    'trunc' {
                        [C1Math]::Truncate($value)
                    }
                    'round' {
                        [C1Math]::Round($value, 0, [MidpointRounding]::AwayFromZero)
                    }
                    'fabs' {
                        [Math]::Abs($value)
                    }
                }
                $expectedBits = MathExpectedBits $expected $single
                $code += "b.f=$operation$suffix(a.f);if(b.u!=$expectedBits)return " + ($i + 1) + ';'
            }
            $whole = [C1Math]::Truncate($value)
            $fraction = $value - $whole
            if ($fraction -eq 0.0) {
                $fraction = [C1Math]::CopySign(0.0, $value)
            }
            $wholeBits = MathExpectedBits $whole $single
            $fractionBits = MathExpectedBits $fraction $single
            $code += "b.f=modf$suffix(a.f,&c.f);if(b.u!=$fractionBits||c.u!=$wholeBits)return " + ($i + 1) + ';'
            $exponent = if ($value -eq 0.0) {
                0
            }
            else {
                [C1Math]::ILogB($value) + 1
            }
            $mantissa = [C1Math]::ScaleB($value, - $exponent)
            $mantissaBits = MathExpectedBits $mantissa $single
            $code += "b.f=frexp$suffix(a.f,&e);if(b.u!=$mantissaBits||e!=$exponent)return " + ($i + 1) + ';'
        }
        $code += 'return 0;}'
        MathCheck "math-oracle-$suffix-$batch" ($prefix + $code)
    }
}
