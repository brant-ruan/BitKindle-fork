# .NET arithmetic produces independent expected TEXT; only custom C tools emit PE.
$values = [Collections.Generic.List[System.Numerics.BigInteger]]::new()
foreach ($n in '0', '1', '7', '8', '15', '16', '255', '256', '2147483647', '2147483648', '4294967295', '4294967296', '9223372036854775807', '9223372036854775808', '18446744073709551615') {
    $values.Add([System.Numerics.BigInteger]::Parse($n))
}
$mod = [System.Numerics.BigInteger]::Pow(2, 64)
$x = [System.Numerics.BigInteger]12345
for ($i = 0; $i -lt 113; $i++) {
    $x = ($x * [System.Numerics.BigInteger]::Parse('6364136223846793005') + 1) % $mod
    $values.Add($x)
}
$program = [Text.StringBuilder]::new()
[void]$program.AppendLine('#include "src/bootstrap-c99/base/lib/runtime.c"')
[void]$program.AppendLine('int main(void){char b[128];')
foreach ($n in $values) {
    $decimal = $n.ToString([Globalization.CultureInfo]::InvariantCulture)
    $hex = $n.ToString('x').TrimStart('0')
    if (-not $hex) {
        $hex = '0'
    }
    $hexExpected = if ($n.IsZero) {
        '0' * 22
    }
    else {
        '0x' + $hex.PadLeft(20, '0')
    }
    $q = $n
    $octal = ''
    do {
        $octal = ([int]($q % 8)).ToString() + $octal
        $q = [System.Numerics.BigInteger]::Divide($q, 8)
    }while ($q -gt 0)
    $octal = $octal.PadLeft(24, '0')
    $signed = if ($n -ge ($mod / 2)) {
        $n - $mod
    }
    else {
        $n
    }
    $sign = if ($signed.Sign -lt 0) {
        '-'
    }
    else {
        '+'
    }
    $signedExpected = $sign + [System.Numerics.BigInteger]::Abs($signed).ToString().PadLeft(23, '0')
    $literal = $decimal + 'ULL'
    [void]$program.AppendLine(('if(snprintf(b,128,"%#022llx",' + $literal + ')!=22||strcmp(b,"' + $hexExpected + '"))return 1;'))
    [void]$program.AppendLine(('if(snprintf(b,128,"%24llu",' + $literal + ')!=24||strcmp(b,"' + $decimal.PadLeft(24) + '"))return 2;'))
    [void]$program.AppendLine(('if(snprintf(b,128,"%.24llo",' + $literal + ')!=24||strcmp(b,"' + $octal + '"))return 3;'))
    [void]$program.AppendLine(('if(snprintf(b,128,"%+024lld",(long long)' + $literal + ')!=24||strcmp(b,"' + $signedExpected + '"))return 4;'))
}
[void]$program.AppendLine('return 0;}')
Check 'format-512-independent-integer-oracles' $program.ToString()
