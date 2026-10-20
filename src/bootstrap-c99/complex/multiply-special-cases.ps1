. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$bits = 'double frombits(unsigned long long n){double v;unsigned char *s=(unsigned char*)&n,*d=(unsigned char*)&v;int i;for(i=0;i<8;i++)d[i]=s[i];return v;}'
$positive = [ordered]@{
    overflownan = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(0x1.fffffffffffffp1023,q)*z(2,-2);return (double)a!=f||im(a)!=-f;}'
    overflowright = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0xfff8000000000000ULL);double _Complex a=z(2,3)*z(q,-0x1.fffffffffffffp1023);return (double)a!=f||im(a)!=-f;}'
    overflowfloat = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);float _Complex a=(float _Complex)z(-0x1.fffffep127,q),b=(float _Complex)z(2,-2),c=a*b;return (float)c!=-f||im(c)!=f;}'
    overflowlong = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);long double _Complex a=(long double _Complex)z(q,0x1.fffffffffffffp1023),b=(long double _Complex)z(2,-2),c=a*b;return (long double)c!=f||im(c)!=f;}'
    infnan = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(f,q)*z(1,1);return (double)a!=f||im(a)!=f;}'
    rightinf = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(1,-1)*z(q,-f);return (double)a!=-f||im(a)!=-f;}'
    bothinf = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(f,q)*z(q,f);return im(a)!=f;}'
    finitecomponent = 'int main(void){double f=frombits(0x7ff0000000000000ULL);double _Complex a=z(f,1)*z(1,0);return (double)a!=f;}'
    nanonly = 'int main(void){double q=frombits(0x7ff8000000000000ULL);double _Complex a=z(q,1)*z(1,1);return (double)a==(double)a||im(a)==im(a);}'
    floatinf = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);float _Complex a=(float _Complex)z(-f,q),b=(float _Complex)z(1,-1),c=a*b;return (float)c!=-f||im(c)!=f;}'
    longinf = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);long double _Complex a=(long double _Complex)z(f,q),b=(long double _Complex)z(1,1),c=a*b;return (long double)c!=f||im(c)!=f;}'
    alias = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(f,q),b=z(1,1),c=a*b;return (double)a!=f||im(a)==im(a)||b!=z(1,1)||(double)c!=f||im(c)!=f;}'
    signgrid = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q;int s,n,c,d,k;double _Complex a,b,p;for(s=-1;s<=1;s+=2)for(n=0;n<2;n++)for(c=-3;c<=3;c++)for(d=-3;d<=3;d++)if(c&&d){q=frombits(n?0xfff8000000000000ULL:0x7ff8000000000000ULL);a=z(s*f,q);b=z(c,d);for(k=0;k<2;k++){p=k?b*a:a*b;if((double)p!=(s*c<0?-f:f)||im(p)!=(s*d<0?-f:f))return 1;}a=z(q,s*f);for(k=0;k<2;k++){p=k?b*a:a*b;if((double)p!=(s*d<0?f:-f)||im(p)!=(s*c<0?-f:f))return 2;}}return 0;}'
}
$positive['overflowgrid'] = $positive['signgrid'].Replace('if(c&&d)', 'if((c<=-2||c>=2)&&(d<=-2||d>=2))').Replace('s*f', 's*0x1.fffffffffffffp1023')
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $bits + $positive[$n]
}
$negative = [ordered]@{}

