$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $wide = if ($mode -eq 'c99') {
            8
        }
        else {
            4
        }
        $signed = if ($mode -eq 'c99') {
            1
        }
        else {
            0
        }
        foreach ($spelling in '2147483648', '4294967295', '2147483648L', '4294967295l') {
            Check "integer-$mode-$spelling-size" "int main(void){return sizeof($spelling);}" $wide
            Check "integer-$mode-$spelling-signed" "int main(void){return -($spelling)<0;}" $signed
        }
        foreach ($spelling in '0x80000000', '0xffffffff', '020000000000', '037777777777', '2147483648U', '4294967295UL') {
            Check "integer-$mode-$spelling-unsigned" "int main(void){return sizeof($spelling)==4 && -($spelling)>0;}" 1
        }
        foreach ($spelling in '4294967296', '4294967296L', '4294967296U', '0x100000000', '9223372036854775807', '18446744073709551615ULL', '0xffffffffffffffffLL', '1ll', '1uLL', '1LLu') {
            Check "integer-$mode-$spelling-wide" "int main(void){return sizeof($spelling);}" 8
        }
        Check "integer-$mode-static-conversion" 'long long n=-2147483648;int main(void){return n<0;}' $signed
        Check "integer-$mode-runtime-conversion" 'int main(void){long long n=-2147483648;return n<0;}' $signed
        PpCheck "integer-$mode-preprocessor-intmax" @('#if 2147483648 > 0 && -2147483648 < 0', 'int main(void){return 7;}', '#else', '#error wrong preprocessing integer type', '#endif') 7
        Check "integer-$mode-decimal-overflow" 'int main(void){return 9223372036854775808;}' 0 $true
        Check "integer-$mode-long-overflow" 'int main(void){return 9223372036854775808L;}' 0 $true
        Check "integer-$mode-uint64-overflow" 'int main(void){return 18446744073709551616ULL;}' 0 $true
    }
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $null, 'Process')
    Check 'integer-default-c89' 'int main(void){return sizeof(2147483648);}' 4
    foreach ($mode in 'c11', 'C99', 'invalid-long-setting') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        Check "integer-reject-mode-$mode" 'int main(void){return 0;}' 0 $true
        $message = [IO.File]::ReadAllText((Join-Path $ScratchDirectory "integer-reject-mode-$mode.err"))
        if ($message -cne "C1 error: C1_STANDARD must be c89 or c99`n") {
            throw 'Missing mode diagnostic'
        }
    }
    [Environment]::SetEnvironmentVariable('C1_STANDARD', 'c99', 'Process')
    # Same production source, compiled twice by custom tools under the C99 selector.
    $own = Join-Path $PSScriptRoot '../c1/compiler.c'
    $generation1 = Join-Path $ScratchDirectory 'integer-c99-self1.exe'
    $generation2 = Join-Path $ScratchDirectory 'integer-c99-self2.exe'
    $selfError = Join-Path $ScratchDirectory 'integer-c99-self.err'
    if ((Native $Compiler $own $generation1 $selfError) -ne 0) {
        throw 'C99 first self compilation failed'
    }
    if ((Native $generation1 $own $generation2 $selfError) -ne 0) {
        throw 'C99 second self compilation failed'
    }
    $one = [IO.File]::ReadAllBytes($generation1)
    $two = [IO.File]::ReadAllBytes($generation2)
    if ($one.Length -ne $two.Length) {
        throw 'C99 self compilation length mismatch'
    }
    for ($i = 0; $i -lt $one.Length; $i++) {
        if ($one[$i] -ne $two[$i]) {
            throw "C99 self compilation mismatch at $i"
        }
    }
    $script:passed++
    'PASS C1 integer-c99-self-twice'
}
finally {
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
