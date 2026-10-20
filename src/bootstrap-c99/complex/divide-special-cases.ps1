. (Join-Path $PSScriptRoot 'axis-cases.ps1')
$positive = [ordered]@{
    infinitenumerator = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(f,q)/z(1,1);return (double)a!=f||im(a)!=-f;}'
    infinitedenominator = 'int main(void){double f=frombits(0x7ff0000000000000ULL);double _Complex a=z(2,3)/z(f,f);return a!=0;}'
    nandivisor = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);double _Complex a=z(2,3)/z(q,-f);double r=(double)a,i=im(a);return r!=0||i!=0||1/r!=-f||1/i!=f;}'
    maxnumerator = 'int main(void){double f=frombits(0x7ff0000000000000ULL),m=0x1.fffffffffffffp1023;double _Complex a=z(m,m)/z(f,f);return a!=0;}'
    maxdenominator = 'int main(void){double f=frombits(0x7ff0000000000000ULL),m=0x1.fffffffffffffp1023;double _Complex a=z(f,f)/z(m,-m);return im(a)!=f;}'
    floatinfinite = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);float _Complex a=(float _Complex)z(q,-f),b=(float _Complex)z(1,1),c=a/b;return (float)c!=-f||im(c)!=-f;}'
    longinfinite = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q=frombits(0x7ff8000000000000ULL);long double _Complex a=(long double _Complex)z(2,3),b=(long double _Complex)z(f,q),c=a/b;return c!=0;}'
    nanonly = 'int main(void){double q=frombits(0x7ff8000000000000ULL);double _Complex a=z(1,q)/z(1,1);return (double)a==(double)a||im(a)==im(a);}'
    bothinfinite = 'int main(void){double f=frombits(0x7ff0000000000000ULL);double _Complex a=z(f,f)/z(f,f);return (double)a==(double)a||im(a)==im(a);}'
    signgrid = 'int main(void){double f=frombits(0x7ff0000000000000ULL),q,r,t;int s,n,c,d;double _Complex a,b,p;for(s=-1;s<=1;s+=2)for(n=0;n<2;n++)for(c=-3;c<=3;c++)for(d=-3;d<=3;d++)if(c&&d){q=frombits(n?0xfff8000000000000ULL:0x7ff8000000000000ULL);a=z(s*f,q);b=z(c,d);p=a/b;if((double)p!=(s*c<0?-f:f)||im(p)!=(s*d<0?f:-f))return 1;p=b/a;r=(double)p;t=im(p);if(r!=0||t!=0||1/r!=(s*c<0?-f:f)||1/t!=(s*d<0?-f:f))return 2;a=z(q,s*f);p=a/b;if((double)p!=(s*d<0?-f:f)||im(p)!=(s*c<0?-f:f))return 3;p=b/a;r=(double)p;t=im(p);if(r!=0||t!=0||1/r!=(s*d<0?-f:f)||1/t!=(s*c<0?f:-f))return 4;}return 0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $bits + $positive[$n]
}
$negative = [ordered]@{}
