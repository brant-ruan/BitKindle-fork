. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$bits = 'double frombits(unsigned long long n){double v;unsigned char *s=(unsigned char*)&n,*d=(unsigned char*)&v;int i;for(i=0;i<8;i++)d[i]=s[i];return v;}'
$positive = [ordered]@{
    infinitenumerator = 'int main(void){double inf=frombits(0x7ff0000000000000ULL);double _Complex a=z(inf,6)/z(2,0);return (double)a!=inf||im(a)!=3;}'
    imaginaryinf = 'int main(void){double inf=frombits(0x7ff0000000000000ULL);double _Complex a=z(6,inf)/z(0,2);return (double)a!=inf||im(a)!=-3;}'
    infiniteboth = 'int main(void){double inf=frombits(0x7ff0000000000000ULL);double _Complex a=z(inf,inf)/z(2,0);return (double)a!=inf||im(a)!=inf;}'
    realsubnormal = 'int main(void){double _Complex a=z(0x1p-1074,0x1p-1074)/z(0x1p-1074,0);return (double)a!=1||im(a)!=1;}'
    imaginarysubnormal = 'int main(void){double _Complex a=z(0x1p-1074,0x1p-1074)/z(0,0x1p-1074);return (double)a!=1||im(a)!=-1;}'
    floatmin = 'int main(void){float _Complex a=(float _Complex)z(0x1p-149,0x1p-149),b=(float _Complex)z(0,0x1p-149),c=a/b;return (float)c!=1||im(c)!=-1;}'
    variedscale = 'int main(void){double _Complex a=z(0x1.fffffffffffffp1023,0x1p-1074)/z(1,0);return (double)a!=0x1.fffffffffffffp1023||im(a)!=0x1p-1074;}'
    infinitedivisor = 'int main(void){double inf=frombits(0x7ff0000000000000ULL);return z(2,3)/z(inf,0)!=0||z(2,3)/z(0,inf)!=0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $bits + $positive[$n]
}
$negative = [ordered]@{}
