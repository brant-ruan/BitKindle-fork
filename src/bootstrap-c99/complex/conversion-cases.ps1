$positive = [ordered]@{
    calls = 'double _Complex f(double _Complex z){return z;}float _Complex g(float _Complex z){return z;}int main(void){double _Complex z=f(3.5);float _Complex y=g(z);return (double)z!=3.5||(float)y!=3.5f;}'
    negzeros = 'int main(void){double a[2]={-0.0,-0.0};double _Complex z;unsigned char *s=(unsigned char*)a,*d=(unsigned char*)&z;int i;for(i=0;i<sizeof z;i++)d[i]=s[i];return (_Bool)z||!!z||(z?1:0);}'
    imaginarynan = 'int main(void){unsigned long long a[2]={0,0x7ff8000000000000ULL};double _Complex z;unsigned char *s=(unsigned char*)a,*d=(unsigned char*)&z;int i;for(i=0;i<sizeof z;i++)d[i]=s[i];return !z||!(_Bool)z||(double)z!=0;}'
    positivezero = 'int main(void){double _Complex z;unsigned long long a[2];unsigned char *s=(unsigned char*)&z,*d=(unsigned char*)a;int i;z=-0.0;for(i=0;i<sizeof z;i++)d[i]=s[i];return a[0]!=0x8000000000000000ULL||a[1]!=0;}'
    real = 'int main(void){double _Complex z;z=3.5;return (double)z!=3.5||(int)z!=3;}'
    widths = 'int main(void){float _Complex f;double _Complex d;long double _Complex l;f=-2.5f;d=f;l=d;f=l;return (float)f!=-2.5f||(double)d!=-2.5||(long double)l!=-2.5L;}'
    initialize = 'int main(void){float _Complex f={4};double _Complex d=f;long double _Complex l={d};return (int)l!=4;}'
    boolzero = 'int main(void){double _Complex z;z=-0.0;return (_Bool)z||!!z;}'
    boolreal = 'int main(void){double _Complex z;z=-2;return !(_Bool)z||!z;}'
    pureimaginary = 'int main(void){double a[2]={0,7};double _Complex z;unsigned char *s=(unsigned char*)a,*d=(unsigned char*)&z;int i;for(i=0;i<sizeof z;i++)d[i]=s[i];if(!z)return 1;if(!(z&&1))return 2;if(!(_Bool)z)return 3;return (double)z!=0;}'
    floatimaginary = 'int main(void){float a[2]={-0.0f,-3.25f};float _Complex f;double _Complex z;float _Complex g;unsigned char *s=(unsigned char*)a,*d=(unsigned char*)&f;int i;for(i=0;i<sizeof f;i++)d[i]=s[i];z=f;g=z;s=(unsigned char*)&g;d=(unsigned char*)a;for(i=0;i<sizeof g;i++)d[i]=s[i];return a[0]!=0||a[1]!=-3.25f||!g;}'
    discard = 'int main(void){double _Complex z;z=2;(void)z;return 0;}'
    unsignedvalue = 'int main(void){unsigned long long n=18446744073709551615ULL;double _Complex z;z=n;return (double)z!=18446744073709551616.0;}'
    copyimaginary = 'int main(void){double a[2]={1,-9};double b[2];double _Complex x,y;unsigned char *s=(unsigned char*)a,*d=(unsigned char*)&x;int i;for(i=0;i<sizeof x;i++)d[i]=s[i];y=x;s=(unsigned char*)&y;d=(unsigned char*)b;for(i=0;i<sizeof y;i++)d[i]=s[i];return b[0]!=1||b[1]!=-9;}'
}
$negative = [ordered]@{
    pointerto = 'int main(void){int *p=0;double _Complex z;z=(double _Complex)p;return 0;}'
    pointerfrom = 'int main(void){double _Complex z;int *p;z=0;p=(int*)z;return 0;}'
    recordto = 'struct S {int x;};int main(void){struct S s;double _Complex z;z=(double _Complex)s;return 0;}'
    recordfrom = 'struct S {int x;};int main(void){struct S s;double _Complex z;z=1;s=z;return 0;}'
}
