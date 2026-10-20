
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = '#include "src/bootstrap-c99/base/lib/math-sqrt.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($v in @(@('double', '', 'unsigned long long', '0x1p-1074', '0x1p-537'), @('float', 'f', 'unsigned int', '0x1p-148f', '0x1p-74f'), @('long double', 'l', 'unsigned long long', '0x1p-1074L', '0x1p-537L'))) {
    $t = $v[0]
    $s = $v[1]
    $u = $v[2]
    $code = "int main(void){$t a;errno=123;feclearexcept(FE_ALL_EXCEPT);"
    $code += "if(sqrt$s(4)!=2||sqrt$s(0.25)!=0.5||sqrt$s($($v[3]))!=$($v[4]))return 1;"
    $code += "if(errno!=123||fetestexcept(FE_ALL_EXCEPT))return 2;"
    $code += "a=sqrt$s(2);if(a<=1.4||a>=1.5||errno!=123||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 3;return 0;}"
    MathCheck "math-sqrt-$s-basic" ($prefix + $code)
}
MathCheck 'math-sqrt-special-rounding' ($prefix + @'
int main(void){
 union{double f;unsigned long long u;}a,b;
 union{float f;unsigned u;}x,y;fenv_t saved;int i;unsigned control;
 feholdexcept(&saved);errno=123;
 a.u=0x8000000000000000ULL;b.f=sqrt(a.f);if(b.u!=a.u)return 1;
 a.u=0x7ff0000000000000ULL;b.f=sqrt(a.f);if(b.u!=a.u)return 2;
 a.u=0xfff8123456789abcULL;b.f=sqrt(a.f);
 if(b.u!=a.u||errno!=123||fetestexcept(FE_ALL_EXCEPT))return 3;
 a.u=0x7ff0000000000123ULL;b.f=sqrt(a.f);
 if(b.u!=0x7ff8000000000123ULL||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 4;
 feclearexcept(FE_ALL_EXCEPT);b.f=sqrt(-1.0);
 if(b.u!=0x7ff8000000000000ULL||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 5;
 feclearexcept(FE_ALL_EXCEPT);errno=123;a.u=0xfff0000000000000ULL;b.f=sqrt(a.f);
 if(b.u!=0x7ff8000000000000ULL||errno!=EDOM||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 11;
 for(i=0;i<4;i++){
  control=0x9fc1U|((unsigned)i<<13);__c1_fe_write(control);errno=123;
  a.u=1;b.f=sqrt(a.f);
  if(b.u!=0x1e60000000000000ULL||errno!=123||__c1_fe_read()!=control)return 6;
  b.f=sqrt(2.0);
  if(b.u!=(i==0||i==2?0x3ff6a09e667f3bcdULL:0x3ff6a09e667f3bccULL))return 7;
  if(__c1_fe_read()!=(control|FE_INEXACT)||errno!=123)return 8;
  __c1_fe_write(control);x.f=2.0f;y.f=sqrtf(x.f);
  if(y.u!=(i==2?0x3fb504f4U:0x3fb504f3U))return 9;
  if(__c1_fe_read()!=(control|FE_INEXACT))return 10;
  __c1_fe_write(control);x.u=1;y.f=sqrtf(x.f);
  if(y.u!=(i==2?0x1a3504f4U:0x1a3504f3U))return 12;
  if(__c1_fe_read()!=(control|FE_INEXACT))return 13;
 }
 fesetenv(&saved);return 0;
}
'@)
$random = [Random]::new(193586)
foreach ($v in @(@('double', '', 'unsigned long long'), @('float', 'f', 'unsigned int'), @('long double', 'l', 'unsigned long long'))) {
    $t = $v[0]
    $s = $v[1]
    $u = $v[2]
    $single = $s -eq 'f'
    for ($batch = 0; $batch -lt 8; $batch++) {
        $code = "int main(void){union{$t f;$u u;}a,b;"
        for ($i = 0; $i -lt 32; $i++) {
            do {
                $bytes = [byte[]]::new($(if ($single) {
                            4
                        }
                        else {
                            8
                        }))
                $random.NextBytes($bytes)
                $bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -band 127
                $value = if ($single) {
                    [double][BitConverter]::ToSingle($bytes, 0)
                }
                else {
                    [BitConverter]::ToDouble($bytes, 0)
                }
            }while ([double]::IsNaN($value) -or [double]::IsInfinity($value))
            if ($single) {
                $inputBits = [BitConverter]::ToUInt32($bytes, 0)
                $expected = [BitConverter]::ToUInt32([BitConverter]::GetBytes([C1MathF]::Sqrt([single]$value)), 0)
            }
            else {
                $inputBits = [BitConverter]::ToUInt64($bytes, 0)
                $expected = [BitConverter]::ToUInt64([BitConverter]::GetBytes([Math]::Sqrt($value)), 0)
            }
            $code += 'a.u=0x' + $inputBits.ToString('X') + "ULL;b.f=sqrt$s(a.f);if(b.u!=0x" + $expected.ToString('X') + 'ULL)return ' + ($i + 1) + ';'
        }
        $code += 'return 0;}'
        MathCheck "math-sqrt-$s-oracle-$batch" ($prefix + $code)
    }
}
