$header = '#include "src/bootstrap-c99/base/lib/math-bits.c"' + [char]10
foreach ($variant in @(@('double', '', '0x1p-1074', '0x1.fffffffffffffp1023', -1073, 1024), @('float', 'f', '0x1p-149f', '0x1.fffffep127f', -148, 128), @('long double', 'l', '0x1p-1074L', '0x1.fffffffffffffp1023L', -1073, 1024))) {
    $t = $variant[0]
    $s = $variant[1]
    $tiny = $variant[2]
    $max = $variant[3]
    $source = "int main(void){$t a,b;int e;"
    $source += "if(fabs$s(-1.75)!=1.75||copysign$s(2.5,-1.0)!=-2.5)return 1;"
    $source += "a=frexp$s(6.0,&e);if(a!=0.75||e!=3)return 2;"
    $source += "a=frexp$s(-6.0,&e);if(a!=-0.75||e!=3)return 3;"
    $source += "a=frexp$s($tiny,&e);if(a!=0.5||e!=$($variant[4]))return 4;"
    $source += "a=frexp$s($max,&e);if(a<0.5||a>=1.0||e!=$($variant[5]))return 5;"
    $source += "a=modf$s(-3.75,&b);if(a!=-0.75||b!=-3)return 6;"
    $source += "a=modf$s($tiny,&b);if(a!=$tiny||b!=0)return 7;"
    $source += "if(floor$s(1.75)!=1||floor$s(-1.75)!=-2||ceil$s(1.75)!=2||ceil$s(-1.75)!=-1)return 8;"
    $source += "if(trunc$s(1.75)!=1||trunc$s(-1.75)!=-1)return 9;"
    $source += "if(round$s(0.5)!=1||round$s(-0.5)!=-1||round$s(2.5)!=3||round$s(-2.5)!=-3)return 10;"
    $source += "if(round$s(0.49)!=0||round$s(-0.49)!=0)return 11;return 0;}"
    MathCheck "math-$t-basic" ($header + $source)
}
MathCheck 'math-bit-corners' ($header + @'
int main(void){
 union{double f;unsigned long long u;}a,b,c;
 union{float f;unsigned u;}x,y;int e;
 a.u=0x8000000000000000ULL;
 b.f=fabs(a.f);if(b.u)return 1;
 b.f=frexp(a.f,&e);if(b.u!=a.u||e)return 2;
 b.f=modf(a.f,&c.f);if(b.u!=a.u||c.u!=a.u)return 3;
 b.f=ceil(-0.25);if(b.u!=a.u)return 4;
 b.f=trunc(-0.75);if(b.u!=a.u)return 5;
 b.f=round(-0.25);if(b.u!=a.u)return 6;
 a.u=0xfff0000000000000ULL;b.f=modf(a.f,&c.f);
 if(b.u!=0x8000000000000000ULL||c.u!=a.u)return 7;
 b.f=floor(a.f);if(b.u!=a.u)return 8;
 a.u=0xfff8123456789abcULL;b.f=fabs(a.f);
 if(b.u!=0x7ff8123456789abcULL)return 9;
 b.f=copysign(a.f,1.0);if(b.u!=0x7ff8123456789abcULL)return 10;
 b.f=modf(a.f,&c.f);if(b.u!=a.u||c.u!=a.u)return 11;
 b.f=frexp(a.f,&e);if(b.u!=a.u||e)return 12;
 x.u=0x80000000;y.f=ceilf(-0.25f);if(x.u!=y.u)return 13;
 x.u=0xff800000;y.f=modff(x.f,&x.f);
 if(y.u!=0x80000000||x.u!=0xff800000)return 14;
 a.f=0x1.fffffffffffffp51;
 if(floor(a.f)!=0x1.ffffffffffffep51||ceil(a.f)!=0x1p52||round(a.f)!=0x1p52)return 15;
 if(floor(-a.f)!=-0x1p52||ceil(-a.f)!=-0x1.ffffffffffffep51)return 16;
 x.f=0x1.fffffep22f;
 if(floorf(x.f)!=0x1.fffffcp22f||ceilf(x.f)!=0x1p23f)return 17;
 return 0;
}
'@)
MathCheck 'math-environment-independent' ($header + '#include "src/bootstrap-c99/base/lib/fenv.c"' + [char]10 + @'
int main(void){
 fenv_t saved;unsigned int mode;int i,e;
 union{double f;unsigned long long u;}a,b;
 union{float f;unsigned u;}x,y;
 fegetenv(&saved);
 for(i=0;i<4;i++){
  mode=0x9fe5U|((unsigned)i<<13);__c1_fe_write(mode);
  a.u=1;b.f=frexp(a.f,&e);if(e!=-1073||b.f!=0.5)return 1;
  x.u=1;y.f=frexpf(x.f,&e);if(e!=-148||y.f!=0.5f)return 2;
  b.f=ceil(a.f);if(b.f!=1.0)return 3;
  a.u=0x8000000000000001ULL;b.f=floor(a.f);if(b.f!=-1.0)return 4;
  if(round(2.5)!=3||round(-2.5)!=-3)return 5;
  if(__c1_fe_read()!=mode)return 6;
 }
 fesetenv(&saved);return 0;
}
'@)
