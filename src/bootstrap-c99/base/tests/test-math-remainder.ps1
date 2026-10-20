
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = '#include "src/bootstrap-c99/base/lib/math-remainder.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($v in @(@('double', '', 'unsigned long long', '0x1p-1074', '0x1.fffffffffffffp1023', 2), @('float', 'f', 'unsigned int', '0x1p-149f', '0x1.fffffep127f', 0), @('long double', 'l', 'unsigned long long', '0x1p-1074L', '0x1.fffffffffffffp1023L', 2))) {
    $t = $v[0]
    $s = $v[1]
    $u = $v[2]
    $tiny = $v[3]
    $max = $v[4]
    $code = "int main(void){union{$t f;$u u;}a,b,c;int mode;fenv_t saved;fegetenv(&saved);"
    $code += "for(mode=0;mode<4;mode++){fesetround(mode<<13);feclearexcept(FE_ALL_EXCEPT);errno=123;"
    $code += "if(fmod$s(5.5,2.0)!=1.5||fmod$s(-5.5,2.0)!=-1.5||fmod$s(5.5,-2.0)!=1.5)return 1;"
    $code += "if(fmod$s($max,3.0)!=$($v[5]))return 2;"
    $code += "a.u=3;b.u=2;c.f=fmod$s(a.f,b.f);if(c.u!=1)return 3;"
    $code += "if(fmod$s($max,$tiny)!=0)return 4;"
    $code += "if(fetestexcept(FE_ALL_EXCEPT)||errno!=123)return 5;}fesetenv(&saved);return 0;}"
    MathCheck "math-fmod-$s-basic" ($prefix + $code)
}
MathCheck 'math-fmod-special' ($prefix + @'
int main(void){
 union{double f;unsigned long long u;}a,b,c;fenv_t saved;unsigned mode;int i;
 feholdexcept(&saved);
 a.u=0x8000000000000000ULL;b.f=3.0;c.f=fmod(a.f,b.f);
 if(c.u!=a.u)return 1;
 a.f=-4.0;b.f=2.0;c.f=fmod(a.f,b.f);if(c.u!=0x8000000000000000ULL)return 2;
 a.f=-1.25;b.u=0x7ff0000000000000ULL;c.f=fmod(a.f,b.f);
 if(c.u!=a.u||fetestexcept(FE_ALL_EXCEPT))return 3;
 a.u=0xfff8123456789abcULL;errno=123;c.f=fmod(a.f,b.f);
 if(c.u!=a.u||errno!=123||fetestexcept(FE_ALL_EXCEPT))return 4;
 a.u=0x7ff0000000000123ULL;c.f=fmod(a.f,b.f);
 if(c.u!=0x7ff8000000000123ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 5;
 feclearexcept(FE_ALL_EXCEPT);errno=123;
 a.u=0x7ff0000000000000ULL;b.f=1.0;c.f=fmod(a.f,b.f);
 if(c.u!=0x7ff8000000000000ULL||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 6;
 feclearexcept(FE_ALL_EXCEPT);errno=123;
 a.f=1.0;b.f=0.0;c.f=fmod(a.f,b.f);
 if(c.u!=0x7ff8000000000000ULL||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 7;
 for(i=0;i<4;i++){
  mode=0x9fe5U|((unsigned)i<<13);__c1_fe_write(mode);errno=123;
  a.u=3;b.u=2;c.f=fmod(a.f,b.f);
  if(c.u!=1||__c1_fe_read()!=mode||errno!=123)return 8;
 }
 fesetenv(&saved);return 0;
}
'@)
$random = [Random]::new(804193)
foreach ($v in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $t = $v[0]
    $s = $v[1]
    $u = $v[2]
    $single = $s -eq 'f'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $code = "int main(void){union{$t f;$u u;}a,b,c;"
        for ($i = 0; $i -lt 32; $i++) {
            $values = @()
            $bits = @()
            for ($j = 0; $j -lt 2; $j++) {
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
                }while ([double]::IsNaN($value) -or [double]::IsInfinity($value) -or ($j -eq 1 -and $value -eq 0.0))
                $values += , $value
                $bits += , $(if ($single) {
                        [BitConverter]::ToUInt32($bytes, 0)
                    }
                    else {
                        [BitConverter]::ToUInt64($bytes, 0)
                    })
            }
            $expected = [double]$values[0] % [double]$values[1]
            if ($expected -eq 0.0) {
                $expected = [C1Math]::CopySign(0.0, $values[0])
            }
            $eb = if ($single) {
                [BitConverter]::ToUInt32([BitConverter]::GetBytes([single]$expected), 0)
            }
            else {
                [BitConverter]::ToUInt64([BitConverter]::GetBytes($expected), 0)
            }
            $code += 'a.u=0x' + $bits[0].ToString('X') + 'ULL;b.u=0x' + $bits[1].ToString('X') + "ULL;c.f=fmod$s(a.f,b.f);if(c.u!=0x" + $eb.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
        }
        $code += 'return 0;}'
        MathCheck "math-fmod-$s-oracle-$batch" ($prefix + $code)
    }
}
