$program = [Text.StringBuilder]::new()
[void]$program.AppendLine('#include "src/bootstrap-c99/base/lib/runtime.c"')
[void]$program.AppendLine('int main(void){unsigned long long u;long long s;int n;')
$mod = [System.Numerics.BigInteger]::Pow(2, 64)
$x = [System.Numerics.BigInteger]12345
for ($i = 0; $i -lt 128; $i++) {
    $x = ($x * [System.Numerics.BigInteger]::Parse('6364136223846793005') + 1) % $mod
    if ($i -eq 0) {
        $x = [System.Numerics.BigInteger]0
    }
    if ($i -eq 1) {
        $x = $mod - 1
    }
    if ($i -eq 2) {
        $x = $mod / 2
    }
    $signed = if ($x -ge $mod / 2) {
        $x - $mod
    }
    else {
        $x
    }
    $decimal = $x.ToString()
    $hex = $x.ToString('x').TrimStart('0')
    if (-not $hex) {
        $hex = '0'
    }
    $octal = ''
    $q = $x
    do {
        $octal = ([int]($q % 8)).ToString() + $octal
        $q = [System.Numerics.BigInteger]::Divide($q, 8)
    }while ($q -gt 0)
    foreach ($item in @(@('llu', $decimal), @('llx', ('0x' + $hex)), @('llo', $octal))) {
        $fmt = $item[0]
        $text = $item[1]
        [void]$program.AppendLine(('if(sscanf("' + $text + '!","%' + $fmt + '%n",&u,&n)!=1||u!=' + $decimal + 'ULL||n!=' + $text.Length + ')return 1;'))
    }
    $text = $signed.ToString()
    [void]$program.AppendLine(('if(sscanf("' + $text + '!","%lld%n",&s,&n)!=1||(unsigned long long)s!=' + $decimal + 'ULL||n!=' + $text.Length + ')return 2;'))
}
[void]$program.AppendLine('return 0;}')
Check 'scan-512-integer-text-oracles' $program.ToString()
