$headerPrefix = "#include `"src/bootstrap-c99/complex/include/complex.h`"`n"
$positive = [ordered]@{
    rank = 'double complex a=(6+8*I)/(1+2*I);int main(void){return sizeof(6+8*I)!=sizeof(float complex)||a!=(double complex)(4.4f-0.8f*I);}'
    imaginary = 'int main(void){double complex a=I;return !a||a==0||(double)a!=0||sizeof I!=sizeof(float complex);}'
    square = 'int main(void){return I*I!=-1||-I*I!=1;} '
    constant = 'double complex a=2+3*I,b=(2+3*I)*(4+5*I),c=(6.0+8*I)/(1+2*I);int main(void){if(a!=2+3*I)return 1;if(b!=-7+22*I)return 2;if(c!=4.4-0.8*I)return 3;return 0;}'
    conditional = 'double complex a=I?3+4*I:0,b=0?I:5-2*I;int main(void){return a!=3+4*I||b!=5-2*I;}'
    bool = 'int a=!!I,b=(_Bool)I,c=I&&1,d=I||0,e=I==0,f=I!=0;int main(void){return a!=1||b!=1||c!=1||d!=1||e!=0||f!=1;}'
    precision = 'long double complex a=(long double complex)I*3;float complex b=(float complex)(2+3*I);int main(void){return a!=3*I||b!=2+3*I;}'
    array = 'double complex a[]={I,2*I,1-I};int main(void){return sizeof a!=48||a[0]!=I||a[1]!=2*I||a[2]!=1-I;}'
    cast = 'double a=(double)(3+4*I);int b=(int)(2.75+5*I);int main(void){return a!=3||b!=2;}'
    local = 'int main(void){double complex a=2+3*I;a*=4+5*I;a/=1+I;return a!=7.5+14.5*I;}'
    macro = 'int main(void){double complex a=_Complex_I;return a!=I;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $headerPrefix + $positive[$n]
}
$negative = [ordered]@{
    assignment = 'int main(void){I=2;return 0;}'
    address = 'int main(void){return &I!=0;}'
    enum = 'enum E{N=(int)I};int main(void){return 0;}'
    bitwise = 'int main(void){return (int)(I&1);}'
    relational = 'int x=I<1;int main(void){return 0;}'
}
foreach ($n in @($negative.Keys)) {
    $negative[$n] = $headerPrefix + $negative[$n]
}
