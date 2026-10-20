
. (Join-Path $PSScriptRoot '../../ps51-compat.ps1')
foreach ($fun in @('sin', 'cos', 'tan')) {
    $rows = for ($i = 0; $i -lt 1024; $i++) {
        $a = ([uint32]$rng.Next(0, 255) -shl 23) -bor [uint32]$rng.Next(1, 8388608)
        if ($i -band 1) {
            $a = $a -bor ([Convert]::ToUInt32('80000000', 16))
        }
        if ($i -eq 0) {
            $a = ([Convert]::ToUInt32('7f7fffff', 16))
        }
        $x = [C1Bits]::UInt32BitsToSingle($a)
        $r = switch ($fun) {
            sin {
                [C1MathF]::Sin($x)
            }cos {
                [C1MathF]::Cos($x)
            }tan {
                [C1MathF]::Tan($x)
            }
        }
        '{0x' + $a.ToString('X') + 'U,0x' + ([C1Bits]::SingleToUInt32Bits($r)).ToString('X') + 'U}'
    }
    Check "trig-$fun-full-exponent-float" ($prefix + 'static unsigned int rows[][2]={' + ($rows -join ',') + '};int main(void){int i;unsigned int d;union{float f;unsigned int u;}x,r;for(i=0;i<1024;i++){x.u=rows[i][0];r.f=' + $fun + 'f(x.f);d=r.u>rows[i][1]?r.u-rows[i][1]:rows[i][1]-r.u;if(d>1)return 1;}return 0;}') 0
    Check "trig-$fun-nan-environment" ($prefix + 'int main(void){union{double f;unsigned long long u;}x,r;union{float f;unsigned int u;}a,b;fenv_t e;x.u=0xfff0000000000035ULL;errno=73;feclearexcept(FE_ALL_EXCEPT);r.f=' + $fun + '(x.f);if(r.u!=0xfff8000000000035ULL||errno!=73||fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 1;a.u=0xffc00035U;feclearexcept(FE_ALL_EXCEPT);b.f=' + $fun + 'f(a.f);if(b.u!=a.u||fetestexcept(FE_ALL_EXCEPT))return 2;fegetenv(&e);e.__mxcsr|=0x8040;fesetenv(&e);fesetround(FE_DOWNWARD);feraiseexcept(FE_DIVBYZERO);r.f=' + $fun + '(1e300);fegetenv(&e);return (e.__mxcsr&0x8040)!=0x8040||fegetround()!=FE_DOWNWARD||fetestexcept(FE_ALL_EXCEPT)!=(FE_DIVBYZERO|FE_INEXACT)||errno!=73;}') 0
}
Check 'trig-tiny-all-modes-both-signs' ($prefix + @'
int main(void){
 int modes[4]={FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO},i,n,k;
 unsigned long long expected;
 union{double f;unsigned long long u;}x,r;
 union{float f;unsigned int u;}a,b;
 for(i=0;i<4;i++)for(n=0;n<2;n++)for(k=0;k<3;k++){
  fesetround(modes[i]);x.u=1ULL|((unsigned long long)n<<63);a.u=1U|((unsigned int)n<<31);
  expected=1;
  if(k==0&&(i==3||(n?i==1:i==2)))expected=0;
  if(k==2&&(n?i==2:i==1))expected=2;
  if(k==1)expected=0x3ff0000000000000ULL-((i==2||i==3)?1:0);else expected|=(unsigned long long)n<<63;
  errno=0;feclearexcept(FE_ALL_EXCEPT);
  r.f=k==0?sin(x.f):k==1?cos(x.f):tan(x.f);
  if(r.u!=expected||errno!=(k==1?0:ERANGE)||fetestexcept(FE_ALL_EXCEPT)!=(k==1?FE_INEXACT:FE_INEXACT|FE_UNDERFLOW))return 1;
  expected=1;
  if(k==0&&(i==3||(n?i==1:i==2)))expected=0;
  if(k==2&&(n?i==2:i==1))expected=2;
  if(k==1)expected=0x3f800000U-((i==2||i==3)?1:0);else expected|=(unsigned int)n<<31;
  errno=0;feclearexcept(FE_ALL_EXCEPT);
  b.f=k==0?sinf(a.f):k==1?cosf(a.f):tanf(a.f);
  if(b.u!=expected||errno!=(k==1?0:ERANGE)||fetestexcept(FE_ALL_EXCEPT)!=(k==1?FE_INEXACT:FE_INEXACT|FE_UNDERFLOW))return 2;
 }
 return 0;
}
'@) 0
