$helpers = 'double _Complex z(double r,double i){double a[2];double _Complex v;unsigned char *s=(unsigned char*)a,*d=(unsigned char*)&v;int n;a[0]=r;a[1]=i;for(n=0;n<sizeof v;n++)d[n]=s[n];return v;}double im(double _Complex v){double a[2];unsigned char *s=(unsigned char*)&v,*d=(unsigned char*)a;int n;for(n=0;n<sizeof v;n++)d[n]=s[n];return a[1];}'
$positive = [ordered]@{
    add = 'int main(void){double _Complex a=z(2,3)+z(4,-7);return (double)a!=6||im(a)!=-4;}'
    subtract = 'int main(void){double _Complex a=z(2,3)-z(4,-7);return (double)a!=-2||im(a)!=10;}'
    unary = 'int main(void){double _Complex a=z(2,3),b=-a,c=+a;return (double)b!=-2||im(b)!=-3||im(a)!=3||im(c)!=3;}'
    mixed = 'int main(void){double _Complex a=z(2,3),b=5-a,c=a-5,d=a+5,e=5+a;return (double)b!=3||im(b)!=-3||(double)c!=-3||im(c)!=3||(double)d!=7||im(d)!=3||(double)e!=7||im(e)!=3;}'
    equality = 'int main(void){double _Complex a=z(2,3);return !(a==z(2,3))||a==z(2,4)||a==z(3,3)||!(a!=2)||!(z(2,0)==2)||2!=z(2,0);}'
    precision = 'int main(void){float _Complex a=(float _Complex)z(2,3);long double _Complex b=(long double _Complex)z(4,5);return sizeof(a+b)!=sizeof(long double _Complex)||im(a+b)!=8||sizeof(a+1.0)!=sizeof(double _Complex)||sizeof(a+1)!=sizeof(float _Complex);}'
    conditional = 'int main(void){int c=0;double _Complex a=c?z(2,3):4,b=!c?z(2,3):4;return a!=4||im(b)!=3;}'
    compound = 'int main(void){double _Complex a=z(2,3);a+=z(4,5);a-=2;return (double)a!=4||im(a)!=8;}'
    nan = 'int main(void){unsigned long long bits=0x7ff8000000000000ULL;double q;unsigned char *s=(unsigned char*)&bits,*d=(unsigned char*)&q;int i;double _Complex a;for(i=0;i<8;i++)d[i]=s[i];a=z(1,q);return a==a||!(a!=a)||a==1;}'
    signedzero = 'int main(void){double _Complex a=z(2,-0.0),b=a+1;double v=im(b);unsigned long long bits;unsigned char *s=(unsigned char*)&v,*d=(unsigned char*)&bits;int i;for(i=0;i<8;i++)d[i]=s[i];return bits!=0x8000000000000000ULL;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $positive[$n]
}
$negative = [ordered]@{
    less = 'int main(void){double _Complex a=1;return a<2;}'
    greater = 'int main(void){double _Complex a=1;return a>2;}'
    remainder = 'int main(void){double _Complex a=1;return a%2;}'
    bitwise = 'int main(void){double _Complex a=1;return a&2;}'
    complement = 'int main(void){double _Complex a=1;return (int)~a;}'
    pointer = 'int main(void){double _Complex a=1;int *p=0;return (int)(a+p);}'
}
