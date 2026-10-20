$positive = [ordered]@{
    casts = 'double _Complex a=(double _Complex)3.5;float _Complex b=(float _Complex)(long double _Complex)-2.5L;int main(void){return a!=3.5||b!=-2.5;}'
    unary = 'double _Complex a=-(double _Complex)3;double _Complex b=+(double _Complex)4;int main(void){return a!=-3||b!=4;}'
    arithmetic = 'double _Complex a=((double _Complex)3+2)*((float _Complex)4-1)/((long double _Complex)2);int main(void){return a!=7.5;}'
    truth = 'int a=!(double _Complex)0,b=(double _Complex)2&&3,c=(double _Complex)0||4;_Bool d=(_Bool)(double _Complex)2;int main(void){return a!=1||b!=1||c!=1||d!=1;}'
    compare = 'int a=(double _Complex)2==2,b=(double _Complex)2!=3,c=(float _Complex)4==(double _Complex)4;int main(void){return a!=1||b!=1||c!=1;}'
    conditional = 'double _Complex a=(double _Complex)1?(float _Complex)2:3,b=(double _Complex)0?2:(long double _Complex)4;int main(void){return a!=2||b!=4;}'
    discardimaginary = 'double a=(double)(-(double _Complex)3.5);int b=(int)(double _Complex)4.5;int main(void){return a!=-3.5||b!=4;}'
    negativeimaginaryzero = 'double _Complex a=-(double _Complex)3;int main(void){unsigned long long b[2];unsigned char *s=(unsigned char*)&a,*d=(unsigned char*)b;int i;for(i=0;i<sizeof a;i++)d[i]=s[i];return b[1]!=0x8000000000000000ULL;}'
    aggregates = 'struct S{double _Complex x;};struct S a={.x=(double _Complex)3+2};float _Complex b[]={(float _Complex)4,-(float _Complex)2};int main(void){return a.x!=5||b[0]!=4||b[1]!=-2;}'
    skipped = 'double _Complex a=1?(double _Complex)2:(double _Complex)1/0;int main(void){return a!=2;}'
}
$negative = [ordered]@{
    relational = 'int a=(double _Complex)2<3;int main(void){return 0;}'
    bitwise = 'int a=(double _Complex)2&3;int main(void){return 0;}'
    pointer = 'double _Complex a=(double _Complex)(void*)0;int main(void){return 0;}'
    nonconstant = 'double x;double _Complex a=(double _Complex)x;int main(void){return 0;}'
    dividezero = 'double _Complex a=(double _Complex)1/0;int main(void){return 0;}'
}
