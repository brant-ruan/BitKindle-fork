. (Join-Path $PSScriptRoot 'axis-cases.ps1')
$positive = [ordered]@{
    cancellation = 'int main(void){double f=frombits(0x7ff0000000000000ULL);double _Complex a=z(0x1.fffffffffffffp1023,0x1.fffffffffffffp1023),b=a*a;return (double)b!=0||im(b)!=f;}'
    floatcancellation = 'int main(void){double f=frombits(0x7ff0000000000000ULL);float _Complex a=(float _Complex)z(0x1.fffffep127,0x1.fffffep127),b=a*a;return (float)b!=0||im(b)!=f;}'
    longcancellation = 'int main(void){double f=frombits(0x7ff0000000000000ULL);long double _Complex a=(long double _Complex)z(0x1.fffffffffffffp1023,0x1.fffffffffffffp1023),b=a*a;return (long double)b!=0||im(b)!=f;}'
    underflow = 'int main(void){double _Complex a=z(0x1p-537,0x1p-537)*z(0x1p-538,0x1p-538);return (double)a!=0||im(a)!=0x1p-1074;}'
    exponentgrid = 'double power(int n){if(n>1023)return frombits(0x7ff0000000000000ULL);if(n< -1074)return 0;return frombits(n< -1022?1ULL<<(n+1074):(unsigned long long)(n+1023)<<52);}int main(void){int e[17]={-1074,-1073,-1022,-1000,-600,-538,-537,-500,-1,0,1,500,537,538,600,1000,1023};int i,j;double a,b;double _Complex c;for(i=0;i<17;i++)for(j=0;j<17;j++){a=power(e[i]);b=power(e[j]);c=z(a,a)*z(b,-b);if((double)c!=power(e[i]+e[j]+1)||im(c)!=0)return 1;}return 0;}'
    floatgrid = 'double power(int n){if(n>127)return frombits(0x7ff0000000000000ULL);if(n< -149)return 0;return frombits((unsigned long long)(n+1023)<<52);}int main(void){int e[13]={-149,-148,-126,-100,-75,-74,-1,0,1,74,75,100,127};int i,j;double a,b;float _Complex c,x,y;for(i=0;i<13;i++)for(j=0;j<13;j++){a=power(e[i]);b=power(e[j]);x=(float _Complex)z(a,a);y=(float _Complex)z(b,-b);c=x*y;if((float)c!=power(e[i]+e[j]+1)||im(c)!=0)return 1;}return 0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $bits + $positive[$n]
}
$negative = [ordered]@{}
