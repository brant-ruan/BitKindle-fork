. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$positive = [ordered]@{
    multiply = 'int main(void){double _Complex a=z(2,3)*z(4,5);return (double)a!=-7||im(a)!=22;}'
    square = 'int main(void){double _Complex a=z(0,1);return a*a!=-1;}'
    mixed = 'int main(void){double _Complex a=z(2,3),b=a*4,c=-2*a;return (double)b!=8||im(b)!=12||(double)c!=-4||im(c)!=-6;}'
    divide = 'int main(void){double _Complex a=z(6,-9)/3,b=z(6,-9)/-3;return (double)a!=2||im(a)!=-3||(double)b!=-2||im(b)!=3;}'
    precision = 'int main(void){float _Complex a=(float _Complex)z(2,3);long double _Complex b=(long double _Complex)z(4,5),c=a*b;return sizeof(a*b)!=sizeof(long double _Complex)||(long double)c!=-7||im(c)!=22||sizeof(a*2)!=sizeof(float _Complex);}'
    compound = 'int main(void){double _Complex a=z(2,3);a*=z(4,5);a/=2;return (double)a!=-3.5||im(a)!=11;}'
    aliased = 'int main(void){double _Complex a=z(2,3);a*=a;return (double)a!=-5||im(a)!=12;}'
    grid = 'int main(void){int a,b,c,d;double _Complex p;for(a=-3;a<=3;a++)for(b=-3;b<=3;b++)for(c=-3;c<=3;c++)for(d=-3;d<=3;d++){p=z(a,b)*z(c,d);if((double)p!=a*c-b*d||im(p)!=a*d+b*c)return 1;}return 0;}'
    signedzero = 'int main(void){double v=im(z(2,-0.0)*2);unsigned long long bits;unsigned char *s=(unsigned char*)&v,*d=(unsigned char*)&bits;int i;for(i=0;i<8;i++)d[i]=s[i];return bits!=0x8000000000000000ULL;}'
    scale = 'int main(void){double _Complex a=z(0x1p500,0x1p500),b=z(0x1p-500,-0x1p-500);return a*b!=2;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $positive[$n]
}
$negative = [ordered]@{
    pointer = 'int main(void){int *p=0;double _Complex a=1;return (int)(a*p);}'
    record = 'struct S{int n;};int main(void){struct S s;double _Complex a=1;return (int)(s*a);}'
    divisor = 'int main(void){int *p=0;double _Complex a=1;return (int)(a/p);}'
}
