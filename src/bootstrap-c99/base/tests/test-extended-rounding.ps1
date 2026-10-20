
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
$rng = [Random]::new(89521)
function RoundBits([double]$x, [bool]$single) {
    if ($single) {
        return [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$x), 0)
    }
    return [BitConverter]::ToUInt64([BitConverter]::GetBytes($x), 0)
}
function RoundExpected([double]$x, [int]$mode) {
    $r = switch ($mode) {
        0 {
            [C1Math]::Round($x, [MidpointRounding]::ToEven)
        }1 {
            [C1Math]::Floor($x)
        }2 {
            [C1Math]::Ceiling($x)
        }3 {
            [C1Math]::Truncate($x)
        }
    }
    if ($r -eq 0) {
        $r = [C1Math]::CopySign(0, $x)
    }
    return [double]$r
}
foreach ($v in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $t = $v[0]
    $s = $v[1]
    $u = $v[2]
    $single = $s -eq 'f'
    for ($mode = 0; $mode -lt 4; $mode++) {
        $values = @(0.0, [C1Math]::CopySign(0, -1), 0.5, -0.5, 1.5, -1.5, 2.5, -2.5, 3.75, -3.75, 0.25, -0.25)
        while ($values.Count -lt 32) {
            $b = [byte[]]::new($(if ($single) {
                        4
                    }
                    else {
                        8
                    }))
            $rng.NextBytes($b)
            $x = if ($single) {
                [double][BitConverter]::ToSingle($b, 0)
            }
            else {
                [BitConverter]::ToDouble($b, 0)
            }
            if (-not [double]::IsNaN($x) -and -not [double]::IsInfinity($x)) {
                $values += , $x
            }
        }
        $src = $prefix + "int main(void){union{$t f;$u u;}a,b;fenv_t saved,mode;fegetenv(&saved);"
        $src += 'mode.__mxcsr=0x9fc4U|' + ($mode -shl 13) + ';errno=123;'
        $i = 0
        foreach ($x in $values) {
            $i++
            $bits = RoundBits $x $single
            $outbits = RoundBits (RoundExpected $x $mode) $single
            $flags = if ($bits -eq $outbits) {
                4
            }
            else {
                36
            }
            $src += 'a.u=0x' + $bits.ToString('X') + 'ULL;fesetenv(&mode);' + "b.f=rint$s(a.f);" + 'if(b.u!=0x' + $outbits.ToString('X') + 'ULL||fetestexcept(FE_ALL_EXCEPT)!=' + $flags + '||errno!=123)return ' + $i + ';'
            $src += 'fesetenv(&mode);' + "b.f=nearbyint$s(a.f);" + 'if(b.u!=0x' + $outbits.ToString('X') + 'ULL||fetestexcept(FE_ALL_EXCEPT)!=4||errno!=123)return ' + $i + ';'
        }
        $src += 'fesetenv(&saved);return 0;}'
        ExtendedCheck "math-rint-$s-$mode" $src
        $src = $prefix + "int main(void){union{$t f;$u u;}a;fenv_t saved,mode;fegetenv(&saved);"
        $src += 'mode.__mxcsr=0x9fc4U|' + ($mode -shl 13) + ';errno=123;'
        for ($i = 0; $i -lt 16; $i++) {
            $x = $rng.Next(-10000, 10001) / 8.0
            $bits = RoundBits $x $single
            $r = [long](RoundExpected $x $mode)
            $away = [long][C1Math]::Round($x, [MidpointRounding]::AwayFromZero)
            $flags = if ($x -eq $r) {
                4
            }
            else {
                36
            }
            $src += 'a.u=0x' + $bits.ToString('X') + 'ULL;'
            foreach ($fn in @('lrint', 'llrint')) {
                $src += 'fesetenv(&mode);if(' + "$fn$s(a.f)!=$r" + 'LL||fetestexcept(FE_ALL_EXCEPT)!=' + $flags + '||errno!=123)return ' + ($i + 1) + ';'
            }
            foreach ($fn in @('lround', 'llround')) {
                $src += 'fesetenv(&mode);if(' + "$fn$s(a.f)!=$away" + 'LL||fetestexcept(FE_ALL_EXCEPT)!=4||errno!=123)return ' + ($i + 1) + ';'
            }
        }
        $src += 'fesetenv(&saved);return 0;}'
        ExtendedCheck "math-round-integer-$s-$mode" $src
    }
    $src = $prefix + "int main(void){union{$t f;$u u;}a,b;fenv_t saved;feholdexcept(&saved);"
    $src += "if(lrint$s(0x1p31)!=(-2147483647L-1)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 1;"
    $src += 'feclearexcept(FE_ALL_EXCEPT);errno=123;'
    $src += "if(llrint$s(-0x1p63)!=(-9223372036854775807LL-1)||errno!=123||fetestexcept(FE_ALL_EXCEPT))return 2;"
    $src += "if(llround$s(0x1p63)!=(-9223372036854775807LL-1)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 3;"
    $src += 'feclearexcept(FE_ALL_EXCEPT);errno=123;'
    $qnan = if ($single) {
        '7FC00000'
    }
    else {
        '7FF8000000000000'
    }
    $snan = if ($single) {
        '7F800000'
    }
    else {
        '7FF0000000000000'
    }
    $src += 'a.u=0x' + $qnan + 'ULL;' + "b.f=nearbyint$s(a.f);" + 'if(b.u!=a.u||fetestexcept(FE_ALL_EXCEPT)||errno!=123)return 4;'
    $src += 'a.u=0x' + $snan + 'ULL+1;' + "b.f=rint$s(a.f);" + 'if(b.u!=(0x' + $qnan + 'ULL+1)||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID||errno!=123)return 5;'
    $src += 'feclearexcept(FE_ALL_EXCEPT);'
    $src += "if(lround$s(a.f)!=(-2147483647L-1)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 6;"
    $src += 'fesetenv(&saved);return 0;}'
    ExtendedCheck "math-round-domain-$s" $src
}
ExtendedCheck 'math-round-integer-edge-environment' ($prefix + @'
int main(void){
 fenv_t saved,mode;int r;long value;double a;
 fegetenv(&saved);
 for(r=0;r<4;r++){
  mode.__mxcsr=0x9fc0U|(r<<13);fesetenv(&mode);errno=123;
  value=lrint(2147483647.5);
  if(r==0||r==2){
   if(value!=(-2147483647L-1)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 1;
  }else if(value!=2147483647L||errno!=123||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 2;
  fesetenv(&mode);errno=123;value=lrint(-2147483648.5);
  if(r==1){
   if(value!=(-2147483647L-1)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 3;
  }else if(value!=(-2147483647L-1)||errno!=123||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 4;
  fesetenv(&mode);errno=123;value=lround(-2147483648.5);
  if(value!=(-2147483647L-1)||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 5;
  mode.__mxcsr|=FE_INEXACT;fesetenv(&mode);
  a=nearbyint(0.5);if(fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 6;
 }
 fesetenv(&saved);return 0;
}
'@)
