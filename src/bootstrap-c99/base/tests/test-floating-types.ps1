$cases = [ordered]@{
    'qualifiers' = 'typedef double D;const D a=1.25;volatile float b=2.5f;double f(const double x,volatile float y){return x+y;}int main(void){const double*p=&a;volatile float*q=&b;*q+=1.0f;return f(*p,*q)!=4.75;}'
    'long-double' = 'double long a=1.25L;typedef long double L;L f(L x){return x+2.5L;}int main(void){L x=a;return sizeof(x)!=8||f(x)!=3.75L;}'
    'array-parameters' = 'double f(float a[],const double b[2]){return a[0]+b[1];}int main(void){float a[2]={1.25f,2};double b[2]={3,4.5};return f(a,b)!=5.75;}'
    'pointer-increment' = 'int main(void){float a[3]={1,2,3};double b[3]={4,5,6};float*p=a;double*q=b;return *p++!=1||*++p!=3||p-a!=2||*q++!=4||*++q!=6||q-b!=2;}'
    'old-prototype' = 'double f(double);double f(x)float x;{return x+0.5;}int main(void){return f(1.25)!=1.75;}'
    'unspecified-compatible' = 'double f();double f(double x){return x+0.25;}int main(void){return f(2.5f)!=2.75;}'
    'old-function-pointer' = 'double f(x)float x;{return x+0.5;}int main(void){double(*p)(double)=f;return p(1.25)!=1.75;}'
    'all-stack' = 'long double f(int a,int b,int c,int d,float e,double g,long double h){return a+b+c+d+e+g+h;}int main(void){return f(1,2,3,4,1.25f,2.5,3.75L)!=17.5L;}'
    'eight-float' = 'float f(float a,float b,float c,float d,float e,float f,float g,float h){return a+2*b+3*c+4*d+5*e+6*f+7*g+8*h;}int main(void){return f(1,2,3,4,5,6,7,8)!=204.0f;}'
    'mixed-promotions' = 'int main(void){char a=-2;unsigned short b=65535;enum E{N=3};float f=0.5f;return a+f!=-1.5f||b+f!=65535.5f||N+f!=3.5f||sizeof(a+f)!=4||sizeof(b+f)!=4;}'
    'usual-types' = 'int main(void){return sizeof(1ULL+1.0f)!=4||sizeof(1.0f+1.0)!=8||sizeof(1+1.0L)!=8||sizeof(1.0f<1.0)!=4||sizeof(1.0L==1)!=4;}'
    'return-conversion' = 'float f(void){return 1.000000059604644775390625;}double g(void){return 16777217;}int main(void){return f()!=1.0f||g()!=16777217.0;}'
    'assignment-value' = 'int main(void){float f;double d;double a=(f=1.000000059604644775390625),b=(d=16777217);return a!=1.0||f!=1.0f||b!=16777217.0;}'
    'nested-calls' = 'double f(double x){return x+1;}float g(float x){return x*2;}int main(void){return f(g(f(g(1.25f))))!=8.0;}'
    'postfix-preservation' = 'int main(void){float f=16777216.0f;double d=0x1p53;float a=f++;double b=d++;return a!=16777216.0f||f!=a||b!=0x1p53||d!=b;}'
    'conditionals-side-effects' = 'int main(void){int i=0;double a=0.0?(i++,2.0):(i++,3.0);float b=-0.0f?(i+=10,2.0f):(i++,4.0f);return i!=2||a!=3||b!=4;}'
    'static-local' = 'double f(void){static double x=1.0/8.0;x+=0.125;return x;}int main(void){return f()!=0.25||f()!=0.375;}'
    'union' = 'union U{float f;double d;long double l;};int main(void){union U u={1.25f};if(u.f!=1.25f)return 1;u.d=2.5;if(u.d!=2.5)return 2;u.l=3.75L;return u.l!=3.75L||sizeof(u)!=8;}'
    'register' = 'double f(register float a,register double b){register long double x=a+b;return x;}int main(void){return f(1.25f,2.5)!=3.75;}'
    'void-cast' = 'int main(void){float f=1.0f;(void)f++;return f!=2.0f;}'
}
foreach ($name in $cases.Keys) {
    FloatingCheck "fp-type-$name" $cases[$name]
}
$bad = @(
    'float float a;int main(void){return 0;}',
    'double double a;int main(void){return 0;}',
    'float double a;int main(void){return 0;}',
    'short double a;int main(void){return 0;}',
    'signed double a;int main(void){return 0;}',
    'int float a;int main(void){return 0;}',
    'double f(double);double f(long double x){return x;}int main(void){return 0;}',
    'float f();float f(float x){return x;}int main(void){return 0;}',
    'double f(float);double f(x)float x;{return x;}int main(void){return 0;}',
    'int main(void){double d;float*p=&d;return 0;}',
    'int main(void){long double d;double*p=&d;return 0;}',
    'float f(float a){return a;}int main(void){double(*p)(double)=f;return 0;}',
    'int main(void){const float a=1;a+=1;return 0;}',
    'int main(void){float a=1;int b=2;b%=a;return 0;}',
    'int main(void){float a=1;int b=2;b<<=a;return 0;}',
    'int main(void){float a=1;return a|2;}',
    'int main(void){float a[3];return a[1.0];}',
    'int main(void){float a=1;return *(a);}',
    'int main(void){float a=1;return a();}',
    'float *p=1.0;int main(void){return 0;}'
)
$i = 0
foreach ($source in $bad) {
    $i++
    FloatingCheck "fp-type-reject-$i" $source 0 $true
}
