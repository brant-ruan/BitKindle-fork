. (Join-Path $PSScriptRoot 'axis-cases.ps1')
$positive = [ordered]@{
    numerator = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m,m)/z(1,1);return (double)a!=m||im(a)!=0;}'
    floatnumerator = 'int main(void){float m=0x1.fffffep127f;float _Complex a=(float _Complex)z(m,m),b=(float _Complex)z(1,1),c=a/b;return (float)c!=m||im(c)!=0;}'
    longnumerator = 'int main(void){long double m=0x1.fffffffffffffp1023L;long double _Complex a=(long double _Complex)z(m,m),b=(long double _Complex)z(1,1),c=a/b;return (long double)c!=m||im(c)!=0;}'
    subnormal = 'int main(void){double _Complex a=z(0x1p-1074,0x1p-1074)/z(0x1p-1074,0x1p-1073);return (double)a!=0.6||im(a)!=-0.2;}'
    floatsubnormal = 'int main(void){float _Complex a=(float _Complex)z(0x1p-149,0x1p-149),b=(float _Complex)z(0x1p-149,0x1p-148),c=a/b;return (float)c!=0.6f||(float)im(c)!=-0.2f;}'
    disparate = 'int main(void){double m=0x1.fffffffffffffp1023;double _Complex a=z(m,m)/z(2,2);return (double)a!=m/2||im(a)!=0;}'
    exponentgrid = 'double power(int n){if(n>1023)return frombits(0x7ff0000000000000ULL);if(n< -1074)return 0;return frombits(n< -1022?1ULL<<(n+1074):(unsigned long long)(n+1023)<<52);}int main(void){int e[17]={-1074,-1073,-1022,-1000,-600,-538,-537,-500,-1,0,1,500,537,538,600,1000,1023};int i,j;double a,b;double _Complex c;for(i=0;i<17;i++)for(j=0;j<17;j++){a=power(e[i]);b=power(e[j]);c=z(a,a)/z(b,b);if((double)c!=power(e[i]-e[j])||im(c)!=0)return 1;c=z(a,a)/z(b,-b);if((double)c!=0||im(c)!=power(e[i]-e[j]))return 2;}return 0;}'
    floatgrid = 'double power(int n){if(n>127)return frombits(0x7ff0000000000000ULL);if(n< -149)return 0;return frombits((unsigned long long)(n+1023)<<52);}int main(void){int e[13]={-149,-148,-126,-100,-75,-74,-1,0,1,74,75,100,127};int i,j;double a,b;float _Complex x,y,c;for(i=0;i<13;i++)for(j=0;j<13;j++){a=power(e[i]);b=power(e[j]);x=(float _Complex)z(a,a);y=(float _Complex)z(b,b);c=x/y;if((float)c!=power(e[i]-e[j])||im(c)!=0)return 1;y=(float _Complex)z(b,-b);c=x/y;if((float)c!=0||im(c)!=power(e[i]-e[j]))return 2;}return 0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $bits + $positive[$n]
}
$negative = [ordered]@{}
