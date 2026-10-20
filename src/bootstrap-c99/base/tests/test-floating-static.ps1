$cases = [ordered]@{
    'arithmetic' = 'float a=1.25f+2.5f,b=9.0f/4.0f,c=2.5f*3.0f,d=1.0f-3.0f;double e=1.25+2.5,f=9.0/4.0,g=2.5*3.0,h=1.0-3.0;int main(void){return a!=3.75f||b!=2.25f||c!=7.5f||d!=-2.0f||e!=3.75||f!=2.25||g!=7.5||h!=-2.0;}'
    'mixed' = 'double a=1+2.5,b=2.5+1,c=~0ULL+0.0;float d=(float)1.25+(float)2;int main(void){return a!=3.5||b!=3.5||c!=0x1p64||d!=3.25f;}'
    'cast' = 'int a=(int)-123.75;unsigned b=(unsigned)4294967295.0;long long c=(long long)-0x1p63;unsigned long long d=(unsigned long long)0x1.fffffffffffffp63;float e=(float)1.000000059604644775390625;double f=(double)0x1p-149f;int main(void){return a!=-123||b!=4294967295U||c!=(-9223372036854775807LL-1)||d!=0xfffffffffffff800ULL||e!=1.0f||f!=0x1p-149;}'
    'conditional' = 'double a=1?2.5f:3.0,b=0?2.5f:3.0;float c=-0.0?4.0f:5;long double d=1?3:2.0L;int main(void){return a!=2.5||b!=3.0||c!=5.0f||d!=3.0L;}'
    'truth' = 'int a=-0.0&&1,b=-0.0||0,c=!0.0f,d=!(-0.0f),e=1.0&&-2.0,f=0.0||1.0;_Bool g=(_Bool)-0.0,h=(_Bool)1e-100;int main(void){return a||b||c!=1||d!=1||e!=1||f!=1||g||h!=1;}'
    'comparison' = 'int a=1.0<2.0,b=1.0<=1.0,c=2.0>1.0,d=2.0>=2.0,e=0.0==-0.0,f=1.0!=2.0,g=-2.0<-1.0,h=-2.0>-1.0;int main(void){return a!=1||b!=1||c!=1||d!=1||e!=1||f!=1||g!=1||h;}'
    'inactive' = 'double a=1?2.5:1.0/0.0,b=0?1e308*1e308:3.5;int c=0&&(int)1e300,d=1||(int)1e300,e=0.0&&(1/0),f=1.0||(1/0);int main(void){return a!=2.5||b!=3.5||c||d!=1||e||f!=1;}'
    'rounding' = 'float a=1.0f+0x1p-24f,b=0x1.000002p0f+0x1p-24f,c=0x1p-149f/2.0f,d=0x1.8p-148f/2.0f;double e=1.0+0x1p-53,f=0x1.0000000000001p0+0x1p-53;int main(void){return a!=1.0f||b!=0x1.000004p0f||c!=0.0f||d!=0x1p-148f||e!=1.0||f!=0x1.0000000000002p0;}'
    'aggregate' = 'struct S{float a;double b;};struct S s[2]={{1.5f+2.0f,3*1.25},{(float)(2.0/4.0),1?3.5:4}};int main(void){return s[0].a!=3.5f||s[0].b!=3.75||s[1].a!=0.5f||s[1].b!=3.5;}'
    'nested' = 'double a=(1.5+2.5)*(3.0-1.0)/(0.5+1.5);int b=(int)((double)(int)2.75+3.5);int main(void){return a!=4.0||b!=5;}'
    'signed-zero' = 'double a=-0.0+-0.0,b=-0.0*1.0;float c=(float)-0.0;int main(void){union{double d;unsigned long long u;}x;union{float f;unsigned u;}y;x.d=a;if(x.u!=0x8000000000000000ULL)return 1;x.d=b;y.f=c;return x.u!=0x8000000000000000ULL||y.u!=0x80000000;}'
    'implicit' = 'float a=1.000000059604644775390625;unsigned long long b=0x1.fffffffffffffp63;int c=-123.75;_Bool d=-0.0;struct S{int x:8;unsigned y:8;};struct S s={-12.75,255.75};int main(void){return a!=1.0f||b!=0xfffffffffffff800ULL||c!=-123||d||s.x!=-12||s.y!=255;}'
}
foreach ($name in $cases.Keys) {
    FloatingCheck "fp-static-$name" $cases[$name]
}
$bad = @(
    'double a=1.0/0.0;int main(void){return 0;}',
    'float a=0x1.fffffep127f*2.0f;int main(void){return 0;}',
    'int a=(int)0x1p31;int main(void){return 0;}',
    'unsigned a=(unsigned)-1.0;int main(void){return 0;}',
    'unsigned long long a=(unsigned long long)0x1p64;int main(void){return 0;}',
    'float a=(float)1e300;int main(void){return 0;}',
    'double a=1.0%2;int main(void){return 0;}',
    'double a=1.0<<1;int main(void){return 0;}',
    'int *a=(int*)0.0;int main(void){return 0;}',
    'double a=(double)(int*)0;int main(void){return 0;}',
    'int *a=1?0.0:(int*)0;int main(void){return 0;}'
    'float a=1e300;int main(void){return 0;}',
    'unsigned a=-1.0;int main(void){return 0;}',
    'int a=0x1p31;int main(void){return 0;}',
    'int *a=0.0;int main(void){return 0;}'
)
$i = 0
foreach ($source in $bad) {
    $i++
    FloatingCheck "fp-static-reject-$i" $source 0 $true
}
