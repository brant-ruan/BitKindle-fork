$cases = [ordered]@{
    'enum' = 'enum{A=(int)3.75,B=(int)((2.75)),C=-(int)1.75,D=(unsigned char)255.9,E=(_Bool)0.25};int main(void){return A!=3||B!=2||C!=-1||D!=255||E!=1;}'
    'bounds' = 'int a[(int)3.75];int main(void){int b[(int)((4.5))];return sizeof(a)!=12||sizeof(b)!=16;}'
    'bitfield' = 'struct S{unsigned a:(int)3.9;int b:(int)(4.25);};int main(void){struct S s={7,-8};return s.a!=7||s.b!=-8;}'
    'case' = 'int main(void){switch(3){case (int)3.75:return 0;default:return 1;}}'
    'inactive-range' = 'enum{A=1?3:(int)1e300,B=0&&(int)1e300};int main(void){return A!=3||B;}'
    'sizeof' = 'enum{A=sizeof(1.0+2.0),B=sizeof((double)1)};int main(void){return A!=8||B!=8;}'
    'null' = 'int main(void){int*p=(int)0.75;int(*f)(void)=(int)((0.5));return p!=0||f!=0;}'
    'null-call' = 'int f(int*p){return p!=0;}int main(void){return f((int)0.25);}'
    'null-conditional' = 'int main(void){int a,*p=&a;return sizeof(*(1?p:(int)0.25))!=4||sizeof(*(1?p:(void*)(int)0.5))!=4;}'
    'negative-zero' = 'int main(void){int*p=-(int)0.5;return p!=0;}'
    'static-null' = 'int*p=(int)((0.75));int(*f)(void)=(int)0.5;int main(void){return p!=0||f!=0;}'
    'static-address' = 'int a[4];int*p=a+(int)2.75,*q=&a[(int)3.5];int main(void){return p!=a+2||q!=a+3;}'
}
foreach ($name in $cases.Keys) {
    FloatingCheck "fp-ice-$name" $cases[$name]
}
$bad = @(
    'enum{A=(int)(1.0+2.0)};int main(void){return 0;}',
    'enum{A=(int)-1.0};int main(void){return 0;}',
    'enum{A=(int)+1.0};int main(void){return 0;}',
    'enum{A=(int)(1?2.0:3.0)};int main(void){return 0;}',
    'enum{A=1?2:3.0};int main(void){return 0;}',
    'enum{A=1?2:(double)3};int main(void){return 0;}',
    'enum{A=(int)(double)3};int main(void){return 0;}',
    'enum{A=(int)0x1p31};int main(void){return 0;}',
    'int a[1.0];int main(void){return 0;}',
    'enum{A=(int)sizeof(char[0x1p-149f])};int main(void){return 0;}',
    'enum{A=1?2:(int)(0,1.0)};int main(void){return 0;}',
    'int main(void){int*p=(int)(1.0-1.0);return 0;}',
    'int main(void){int*p=(int)-0.0;return 0;}',
    'int main(void){int*p=(int)(double)0;return 0;}',
    'int main(void){int*p=(int)1e300;return 0;}'
    'int*p=(int)(1.0-1.0);int main(void){return 0;}',
    'int*p=(int)-0.0;int main(void){return 0;}',
    'int*p=(int)(double)0;int main(void){return 0;}',
    'int*p=1.0==2.0;int main(void){return 0;}',
    'int*p=1?(int)(1.0-1.0):(int*)0;int main(void){return 0;}',
    'int a[4];int*p=a+(int)(1.0+1.0);int main(void){return 0;}',
    'int a[4];int*p=&a[(int)(1.0+1.0)];int main(void){return 0;}'
)
$i = 0
foreach ($source in $bad) {
    $i++
    FloatingCheck "fp-ice-reject-$i" $source 0 $true
}
