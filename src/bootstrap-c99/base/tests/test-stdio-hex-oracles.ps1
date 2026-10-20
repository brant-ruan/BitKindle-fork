
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$program = [Text.StringBuilder]::new()
[void]$program.AppendLine('#include "src/bootstrap-c99/base/lib/runtime.c"')
[void]$program.AppendLine('int main(void){char b[128],*end;union{double f;unsigned long long u;}value;')
$mod = [System.Numerics.BigInteger]::Pow(2, 64)
$cases = 0
# Midrange exact powers of two keep the independent binary scaling operations exact.
foreach ($value in 1.5, 1.96875, 1.0000000000000002, 1.9999999999999998, -1.5, -1.96875, -1.0000000000000002, -1.9999999999999998) {
    foreach ($p in 0..13) {
        $scale = [Math]::Pow(2, 4 * $p)
        foreach ($mode in 'FE_TONEAREST', 'FE_UPWARD', 'FE_DOWNWARD', 'FE_TOWARDZERO') {
            $scaled = $value * $scale
            $rounded = switch ($mode) {
                'FE_TONEAREST' {
                    [C1Math]::Round($scaled, [MidpointRounding]::ToEven)
                } 'FE_UPWARD' {
                    [C1Math]::Ceiling($scaled)
                } 'FE_DOWNWARD' {
                    [C1Math]::Floor($scaled)
                } 'FE_TOWARDZERO' {
                    [C1Math]::Truncate($scaled)
                }
            }
            $expected = $rounded / $scale
            $bits = [System.Numerics.BigInteger][BitConverter]::DoubleToInt64Bits($expected)
            if ($bits.Sign -lt 0) {
                $bits += $mod
            }
            $input = [System.Numerics.BigInteger][BitConverter]::DoubleToInt64Bits($value)
            if ($input.Sign -lt 0) {
                $input += $mod
            }
            [void]$program.AppendLine(('fesetround(' + $mode + ');if(sprintf(b,"%.' + $p + 'a",__fpbits(double,' + $input.ToString() + 'ULL))<0)return 1;value.f=strtod(b,&end);if(*end||value.u!=' + $bits.ToString() + 'ULL)return 2;'))
            $cases++
        }
    }
}
if ($cases -ne 448) {
    throw 'Unexpected oracle case count'
}
[void]$program.AppendLine('return 0;}')
Check 'format-448-hex-rounding-oracles' $program.ToString()
