. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$positive = [ordered]@{
    ordinary = 'int main(void){double _Complex a=z(2,3)/z(4,5);double r=(double)a-23.0/41,i=im(a)-2.0/41;return r>1e-14||r< -1e-14||i>1e-14||i< -1e-14;}'
    realaxis = 'int main(void){double _Complex a=z(6,-9)/z(3,0);return (double)a!=2||im(a)!=-3;}'
    imaginaryaxis = 'int main(void){double _Complex a=z(6,-9)/z(0,3);return (double)a!=-3||im(a)!=-2;}'
    reciprocal = 'int main(void){double _Complex a=10/z(1,2);return (double)a!=2||im(a)!=-4;}'
    compound = 'int main(void){double _Complex a=z(6,8);a/=z(1,2);return (double)a!=4.4||im(a)!=-0.8;}'
    self = 'int main(void){double _Complex a=z(3,4);a/=a;return a!=1;}'
    large = 'int main(void){double _Complex a=z(0x1p600,0x1p600)/z(0x1p600,-0x1p600);return (double)a!=0||im(a)!=1;}'
    small = 'int main(void){double _Complex a=z(0x1p-600,0x1p-600)/z(0x1p-600,-0x1p-600);return (double)a!=0||im(a)!=1;}'
    precision = 'int main(void){float _Complex a=(float _Complex)z(6,-9),b=(float _Complex)z(0,3);long double _Complex c=(long double _Complex)b;return a/b!=(float _Complex)z(-3,-2)||a/c!=(long double _Complex)z(-3,-2)||sizeof(a/c)!=sizeof(long double _Complex);}'
    grid = 'int main(void){int a,b,c,d;double _Complex p;double r,i,den;for(a=-3;a<=3;a++)for(b=-3;b<=3;b++)for(c=-3;c<=3;c++)for(d=-3;d<=3;d++){if(!c&&!d)continue;p=z(a,b)/z(c,d);den=c*c+d*d;r=(double)p-(a*c+b*d)/den;i=im(p)-(b*c-a*d)/den;if(r>1e-13||r< -1e-13||i>1e-13||i< -1e-13)return 1;}return 0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $positive[$n]
}
$negative = [ordered]@{
    pointer = 'int main(void){int *p=0;double _Complex a=1;return (int)(p/a);}'
    record = 'struct S{int n;};int main(void){struct S s;double _Complex a=1;return (int)(a/s);}'
}
