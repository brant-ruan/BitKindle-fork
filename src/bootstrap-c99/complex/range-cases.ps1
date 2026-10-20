. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$positive = [ordered]@{
    maxdouble = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m,m),b=a/a;return b!=1;}'
    maxfloat = 'int main(void){float m=0x1.fffffep127f;float _Complex a=(float _Complex)z(m,m),b=a/a;return b!=1;}'
    maxlong = 'int main(void){long double m=0x1.fffffffffffffp1023L;long double _Complex a=(long double _Complex)z(m,m),b=a/a;return b!=1;}'
    maxnegative = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m,-m),b=z(-m,m);return a/b!=-1;}'
    maximaginary = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m,m),b=z(m,-m),c=a/b;return (double)c!=0||im(c)!=1;}'
    unevendouble = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m,m/2);return a/a!=1;}'
    unevenother = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m/2,m);return a/a!=1;}'
    smallresult = 'int main(void){double _Complex a=z(1,1)/z(0x1p1023,0x1p1023);return (double)a!=0x1p-1023||im(a)!=0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $positive[$n]
}
$negative = [ordered]@{}
