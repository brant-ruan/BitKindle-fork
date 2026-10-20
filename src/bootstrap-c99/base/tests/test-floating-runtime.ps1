$cases = @{
    'double-arithmetic' = 'int main(void){double a=1.5,b=2.25;return a+b!=3.75||b-a!=0.75||a*b!=3.375||b/a!=1.5;}'
    'float-arithmetic' = 'int main(void){float a=1.5f,b=2.25f;return a+b!=3.75f||b-a!=0.75f||a*b!=3.375f||b/a!=1.5f;}'
    'long-double' = 'int main(void){long double a=1.5L,b=2.25L;return sizeof(a)!=8||a+b!=3.75L;}'
    'literal-rounding' = 'union U{float f;unsigned u;};int main(void){union U x;x.f=1.00000005960464477539062500001f;return x.u!=0x3f800001;}'
    'hex-literals' = 'int main(void){double d=0x1.8p+1;float f=0X.8P-1f;return d!=3.0||f!=0.25f;}'
    'global-literals' = 'float a=1.5f;double b=-2.25;long double c=3;int main(void){return a!=1.5||b!=-2.25||c!=3.0L;}'
    'array-record' = 'struct S{double d;float f;};int main(void){struct S a[2]={{1.5,2.25f},{3.75,4.5f}};return a[1].d-a[0].d!=2.25||a[1].f-a[0].f!=2.25f||sizeof(a)!=32;}'
    'float-to-double' = 'int main(void){float a=1.25f;double b=a;return b!=1.25;}'
    'double-to-float' = 'union U{float f;unsigned u;};int main(void){double a=1.000000059604644775390625;union U u;u.f=a;return u.u!=0x3f800000;}'
    'signed-to-float' = 'int main(void){long long a=-123456789;double d=a;int b=-12345;float f=b;return d!=-123456789.0||f!=-12345.0f;}'
    'uint-to-float' = 'int main(void){unsigned a=4294967295U;double d=a;float f=a;return d!=4294967295.0||f!=4294967296.0f;}'
    'ull-to-float' = 'int main(void){unsigned long long a=~0ULL,b=0x8000000000000800ULL;double d=b;return (double)a!=18446744073709551616.0||d!=9223372036854777856.0||(float)a!=0x1p64f;}'
    'float-to-int' = 'int main(void){double a=-123.75,b=4294967295.0;float c=-17.75f;return (int)a!=-123||(unsigned)b!=4294967295U||(int)c!=-17;}'
    'float-to-ull' = 'int main(void){double a=9223372036854777856.0,b=9223372036854774784.0;return (unsigned long long)a!=0x8000000000000800ULL||(unsigned long long)b!=0x7ffffffffffffc00ULL;}'
    'float-to-narrow' = 'int main(void){double a=-127.75,b=65535.75;return (signed char)a!=-127||(unsigned short)b!=65535;}'
    'signed-zero' = 'int main(void){double a=-0.0;float b=-0.0f;if(a||b)return 1;if(a)return 2;if(b)return 3;while(a)return 4;return !a!=1||!b!=1||a!=0.0||b!=0.0f;}'
    'nan-comparisons' = 'int main(void){double z=0.0,n=z/z;return n==n||!(n!=n)||n<z||n<=z||n>z||n>=z||!n||(_Bool)n!=1;}'
    'nan-float' = 'int main(void){float z=0.0f,n=z/z;return n==n||!(n!=n)||n<z||n<=z||n>z||n>=z||!n;}'
    'ordered-comparisons' = 'int main(void){double a=-2.0,b=3.0;return !(a<b)||!(a<=b)||!(b>a)||!(b>=a)||a==b||!(a!=b)||!(a<=a)||!(a>=a);}'
    'conditional' = 'int main(void){float f=2.5f;double d=7.25;return (1?f:d)!=2.5||(0?f:d)!=7.25||sizeof(1?f:1)!=4||sizeof(1?f:d)!=8;}'
    'increments' = 'int main(void){float f=1.5f;double d=2.25;float old=f++;double oldd=d--;if(old!=1.5f||f!=2.5f||oldd!=2.25||d!=1.25)return 1;return ++f!=3.5f||--d!=0.25;}'
    'compound' = 'int main(void){float f=1.5f;double d=5.0;f+=2;f*=2;f-=1;f/=3;d+=f;return f!=2.0f||d!=7.0;}'
    'volatile' = 'int main(void){volatile float a=1.5f;volatile double b=2.25;a+=1;b*=2;return a!=2.5f||b!=4.5;}'
    'direct-call' = 'float f(float a){return a+1.25f;}double g(double a){return a+2.5;}int main(void){return f(2.5f)!=3.75f||g(3.25)!=5.75;}'
    'four-registers' = 'double f(float a,double b,float c,double d){return a+2*b+3*c+4*d;}int main(void){return f(1.0f,2.0,3.0f,4.0)!=30.0;}'
    'mixed-stack' = 'double f(int a,double b,float c,int d,double e,float f){return a+2*b+3*c+4*d+5*e+6*f;}int main(void){return f(1,2.0,3.0f,4,5.0,6.0f)!=91.0;}'
    'indirect-call' = 'double f(float a,double b){return a+b;}int main(void){double(*p)(float,double)=f;return p(1.25f,2.5)!=3.75;}'
    'recursive' = 'double f(int n,double x){if(!n)return x;return f(n-1,x+0.5);}int main(void){return f(9,1.0)!=5.5;}'
    'oldstyle-float' = 'double f();double f(x)float x;{return x+0.5;}int main(void){return f(1.25f)!=1.75;}'
    'oldstyle-mixed' = 'double f();double f(a,b,c,d,e)int a;float b;double c;float d;float e;{return a+2*b+3*c+4*d+5*e;}int main(void){return f(1,2.0f,3.0,4.0f,5.0f)!=55.0;}'
    'aggregate-return' = 'struct S{double a,b;};struct S f(double a,int b,float c,double d,float e){struct S s={a+b+c,d+e};return s;}int main(void){struct S s=f(1.25,2,3.5f,4.75,5.5f);return s.a!=6.75||s.b!=10.25;}'
    'aggregate-parameter' = 'struct S{double a;float b;};double f(struct S s,double x){return s.a+s.b+x;}int main(void){struct S s={1.25,2.5f};return f(s,3.75)!=7.5;}'
    'small-aggregate' = 'struct S{float a;};struct S f(struct S s,float x){s.a+=x;return s;}int main(void){struct S s={1.25f};return f(s,2.5f).a!=3.75f;}'
    'bool-casts' = 'int main(void){return (_Bool)0.0||(_Bool)-0.0f||!(_Bool)1.0||!(_Bool)-1.0f;}'
    'unsigned-small-preserve-rhs' = 'int main(void){unsigned long long a=0x8000000000000800ULL;return a+1.0!=9223372036854777856.0||1.0+a!=9223372036854777856.0;}'
}
foreach ($name in $cases.Keys) {
    FloatingCheck "fp-$name" $cases[$name]
}
$header = '#include "src/bootstrap-c99/base/include/stdarg.h"' + [char]10
if (-not $Stage0NoIncludes) {
    FloatingCheck 'fp-varargs' ($header + 'double f(int n,...){va_list ap;double sum=0.0;int i;va_start(ap,n);for(i=0;i<n;i++)sum+=va_arg(ap,double);va_end(ap);return sum;}int main(void){return f(6,1.0f,2.0,3.0f,4.0,5.0f,6.0)!=21.0;}')
    FloatingCheck 'fp-varargs-fixed-float' ($header + 'double f(float a,int n,...){va_list ap;double sum=a;int i;va_start(ap,n);for(i=0;i<n;i++)sum+=va_arg(ap,double);return sum;}int main(void){return f(1.25f,4,2.0f,3.0,4.0f,5.0)!=15.25;}')
}
$bad = @(
    'float main(void){return 0;}',
    'int main(void){float f=1.0f;return f%2;}',
    'int main(void){double d=1.0;return d&1;}',
    'int main(void){return ~1.0;}',
    'int main(void){return 1.0<<2;}',
    'int main(void){int*p=0;return p+1.0!=p;}',
    'int main(void){int*p=(int*)1.0;return 0;}',
    'int main(void){return (int)(double)(int*)0;}',
    'int main(void){switch(1.0){case 1:return 0;}return 1;}',
    'int a[1.0];int main(void){return 0;}',
    'enum{A=1.0};int main(void){return 0;}',
    'struct S{float a:1;};int main(void){return 0;}',
    'unsigned float x;int main(void){return 0;}',
    'long float x;int main(void){return 0;}',
    'long long double x;int main(void){return 0;}',
    'int main(void){return 1.0u;}',
    'int main(void){return 0x1.0;}',
    'int main(void){int*p=0.0;return p!=0;}'
)
$i = 0
foreach ($source in $bad) {
    $i++
    FloatingCheck "fp-reject-$i" $source 0 $true
}
