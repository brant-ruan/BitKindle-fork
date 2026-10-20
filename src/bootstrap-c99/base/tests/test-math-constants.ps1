$defs = @'
#define INF __fpbits(double,0x7ff0000000000000ULL)
#define QNAN __fpbits(float,0x7fc00000U)
'@
MathCheck 'fpbits-static' ($defs + @'

double a=INF,b=-INF,c=QNAN,d=INF+1.0,e=INF-INF;
float f=INF,g=-INF,h=__fpbits(double,0x7ff0000000000001ULL);
_Bool i=INF,j=QNAN,k=__fpbits(double,0x8000000000000000ULL);
int main(void){
 union {double f;unsigned long long u;} x;
 union {float f;unsigned int u;} y;
 x.f=a;if(x.u!=0x7ff0000000000000ULL)return 1;
 x.f=b;if(x.u!=0xfff0000000000000ULL)return 2;
 x.f=c;if(x.u!=0x7ff8000000000000ULL)return 3;
 if(d!=a||e==e)return 4;
 y.f=f;if(y.u!=0x7f800000U)return 5;
 y.f=g;if(y.u!=0xff800000U)return 6;
 y.f=h;if(y.u!=0x7f800001U)return 7;
 if(!i||!j||k)return 8;
 return 0;
}
'@)
MathCheck 'fpbits-runtime' ($defs + @'

int main(void){
 volatile double x=INF; volatile float y=QNAN;
 if(x!=INF||-x>=0.0||y==y||!y||!x)return 1;
 if(sizeof(INF)!=8||sizeof(QNAN)!=4)return 2;
 if((x-x)==(x-x))return 3;
 if(__fpbits(float,1)!=0x1p-149f)return 4;
 if(__fpbits(double,0x3ff0000000000000ULL)!=1.0)return 5;
 return 0;
}
'@)
MathCheck 'fpbits-ice' 'enum { A=(int)__fpbits(double,0x400c000000000000ULL), B=sizeof(__fpbits(float,0)) }; int a[A];int main(void){return A!=3||B!=4||sizeof(a)!=12;}'
MathCheck 'fpbits-declarations' @'
typedef const float F;
static F a=__fpbits(F,0x3f800000U),b=2.0f;
static int *p=(int)__fpbits(double,0);
int count(void){static double c=__fpbits(double,0x3ff0000000000000ULL),d=2.0;return (int)(c+++d++);}
int main(void){
 if(a!=1||b!=2||p)return 1;
 if(count()!=3||count()!=5)return 2;
 if(__fpbits(double,(unsigned long long)(int)__fpbits(float,0x3f800000U))!=0x1p-1074)return 3;
 return 0;
}
'@
MathCheck 'fpbits-conditional' ($defs + @'

double a=1?INF:0.0,b=0?0.0:QNAN;
int c=INF>1.0,d=QNAN==QNAN,e=!__fpbits(double,0x8000000000000000ULL);
int main(void){return a!=INF||b==b||!c||d||!e;}
'@)
$rejects = @(
    'double x=__fpbits(int,0);',
    'double x=__fpbits(float,0x100000000ULL);',
    'int v; double x=__fpbits(double,v);',
    'double x=__fpbits(double,1.0);',
    'double x=__fpbits(double);',
    'double x=__fpbits(double,);',
    'double x=__fpbits(double*,0);',
    'enum { A=__fpbits(float,0) };',
    'int a[__fpbits(double,0x3ff0000000000000ULL)];',
    'int x=(int)__fpbits(double,0x7ff0000000000000ULL);',
    'int x=(int)__fpbits(float,0x7fc00000U);',
    'int *x=(int)__fpbits(double,0x7ff0000000000000ULL);',
    'double x=1.0/0.0;',
    'float x=0x1.fffffep127f*2.0f;'
    , 'double x=__fpbits(static double,0);'
    , 'double x=__fpbits(extern double,0);'
    , 'double x=__fpbits(typedef double,0);'
    , 'double x=__fpbits(register double,0);'
    , 'double x=__fpbits(auto double,0);'
)
$n = 0
foreach ($src in $rejects) {
    $n++
    MathCheck "fpbits-reject-$n" ($src + 'int main(void){return 0;}') 0 $true
}

MathCheck 'fpbits-conversion-leaves' @'
#include "src/bootstrap-c99/base/c1/fp-convert.c"
#include "src/bootstrap-c99/base/c1/fp-eval.c"
#include "src/bootstrap-c99/base/lib/fenv.c"
int main(void){
 unsigned long long f[8]={0x7f800000ULL,0xff800000ULL,0x7fc00000ULL,
 0xffc00001ULL,0x7f800001ULL,0xffbfffffULL,0x7fffffffULL,0xffffffffULL};
 unsigned long long d[8]={0x7ff0000000000000ULL,0xfff0000000000000ULL,
 0x7ff8000000000000ULL,0xfff8000020000000ULL,0x7ff0000020000000ULL,
 0xfff7ffffe0000000ULL,0x7fffffffe0000000ULL,0xffffffffe0000000ULL};
 fenv_t saved,mode,after;int r,i,t,u;
 fegetenv(&saved);
 for(r=0;r<4;r++){
  mode.__mxcsr=0x9fc0U|(r<<13)|0x25U;fesetenv(&mode);
  for(i=0;i<8;i++)for(t=13;t<=15;t++)for(u=13;u<=15;u++){
   if(fpe_convert(t,u,t==13?f[i]:d[i]))return 1;
   if(fpe_bits!=(u==13?f[i]:d[i]))return 2;
   if(fpe_convert(t,1,t==13?f[i]:d[i])!=1)return 3;
   if(fpe_convert(t,12,t==13?f[i]:d[i])||fpe_bits!=1)return 4;
  }
  fegetenv(&after);if(after.__mxcsr!=mode.__mxcsr)return 5;
 }
 fesetenv(&saved);return 0;
}
'@
MathCheck 'fpbits-math-header' @'
#include "src/bootstrap-c99/base/lib/math-sqrt.c"
#include "src/bootstrap-c99/base/lib/math-classify.c"
#include "src/bootstrap-c99/base/lib/runtime.c"
static double a=HUGE_VAL,b=NAN,c=HUGE_VALF;
static long double d=HUGE_VALL;
static float e=INFINITY,f=NAN;
int main(void){
 fenv_t env;feholdexcept(&env);
 if(sizeof(HUGE_VAL)!=sizeof(double)||sizeof(HUGE_VALF)!=sizeof(float)
 ||sizeof(HUGE_VALL)!=sizeof(long double)||sizeof(NAN)!=sizeof(float))return 1;
 if(!isinf(a)||!isnan(b)||!isinf(c)||!isinf(d)||!isinf(e)||!isnan(f))return 2;
 if(signbit(a)||signbit(e)||!isinf(-HUGE_VAL)||!signbit(-HUGE_VAL))return 3;
 if(!isunordered(NAN,HUGE_VAL)||!isgreater(HUGE_VAL,0x1.fffffffffffffp1023))return 4;
 if(!isinf(sqrt(HUGE_VAL))||!isnan(sqrt(NAN)))return 5;
 if(fetestexcept(FE_ALL_EXCEPT))return 6;
 fesetenv(&env);return 0;
}
'@
