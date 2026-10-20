$positive = [ordered]@{
    globals = 'float _Complex f=1.25f;double _Complex d=-2.5;long double _Complex l={3.75L};int main(void){return f!=1.25f||d!=-2.5||l!=3.75L;}'
    expressions = 'double _Complex a=1.5+2.25,b=1?2.5:9,c=(int)4.5;int main(void){return a!=3.75||b!=2.5||c!=4;}'
    staticlocal = 'int f(void){static double _Complex z=4;int n=(int)z;z+=1;return n;}int main(void){return f()!=4||f()!=5;}'
    array = 'double _Complex a[4]={1,2.5};int main(void){return a[0]!=1||a[1]!=2.5||a[2]!=0||a[3]!=0;}'
    inferred = 'float _Complex a[]={1,2,3};int main(void){return sizeof a!=24||a[2]!=3;}'
    member = 'struct S {int n;double _Complex z;};struct S a={7,-3.5};int main(void){return a.n!=7||a.z!=-3.5;}'
    designated = 'struct S {int n;double _Complex z;};struct S a={.z=3.5,.n=8};double _Complex b[4]={[2]=7};int main(void){return a.n!=8||a.z!=3.5||b[2]!=7||b[0]!=0;}'
    zeros = 'double _Complex a=-0.0;int main(void){unsigned long long b[2];unsigned char *s=(unsigned char*)&a,*d=(unsigned char*)b;int i;for(i=0;i<sizeof a;i++)d[i]=s[i];return b[0]!=0x8000000000000000ULL||b[1]!=0;}'
    nested = 'double _Complex a[2][2]={{1,2},{3,4}};int main(void){return a[0][1]!=2||a[1][0]!=3;}'
    constmember = 'struct S {const double _Complex z;};const struct S a={4};int main(void){return a.z!=4;}'
}
$negative = [ordered]@{
    runtime = 'double x=3;double _Complex z=x;int main(void){return 0;}'
    call = 'double f(void){return 3;}double _Complex z=f();int main(void){return 0;}'
    pair = 'double _Complex z={1,2};int main(void){return 0;}'
    pointer = 'int x;double _Complex z=&x;int main(void){return 0;}'
}
