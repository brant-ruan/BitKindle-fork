. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$values = [Collections.Generic.List[System.Numerics.BigInteger]]::new()
foreach ($v in '0', '1', '4503599627370495', '4503599627370496', '4607182418800017408', '9218868437227405311', '9223372036854775808') {
    $values.Add([System.Numerics.BigInteger]::Parse($v))
}
$mod = [System.Numerics.BigInteger]::Pow(2, 64)
$sign = $mod / 2
$finite = [System.Numerics.BigInteger]::Parse('9218868437227405312')
$x = [System.Numerics.BigInteger]76543
for ($i = 0; $i -lt 121; $i++) {
    $x = ($x * [System.Numerics.BigInteger]::Parse('6364136223846793005') + 1) % $mod
    $bits = $x % $finite
    if ($i % 2) {
        $bits += $sign
    }
    $values.Add($bits)
}
$program = [Text.StringBuilder]::new()
[void]$program.AppendLine('#include "src/bootstrap-c99/base/lib/runtime.c"')
[void]$program.AppendLine('int main(void){char b[1200];')
foreach ($bits in $values) {
    $signed = if ($bits -ge $sign) {
        $bits - $mod
    }
    else {
        $bits
    }
    $value = [BitConverter]::Int64BitsToDouble([long]$signed)
    foreach ($format in 'F17', 'e16', 'g17') {
        $expected = [C1Math]::FormatDecimal($value, $format, [Globalization.CultureInfo]::InvariantCulture)
        $expected = $expected -replace 'e([+-])0([0-9]{2})$', 'e$1$2'
        $cfmt = switch ($format) {
            'F17' {
                '%.17f'
            } 'e16' {
                '%.16e'
            } 'g17' {
                '%.17g'
            }
        }
        [void]$program.AppendLine(('if(sprintf(b,"' + $cfmt + '",__fpbits(double,' + $bits.ToString() + 'ULL))!=' + $expected.Length + '||strcmp(b,"' + $expected + '"))return 1;'))
    }
}
[void]$program.AppendLine('return 0;}')
Check 'format-384-independent-decimal-oracles' $program.ToString()
