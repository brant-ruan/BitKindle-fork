$tag = 'int ff(void){return 1;}int fd(void){return 2;}int fl(void){return 3;}int cf(void){return 4;}int cd(void){return 5;}int cl(void){return 6;}'
$positive = [ordered]@{
    types = 'int main(void){return __c1_tg(1,1.0f,ff,fd,fl,cf,cd,cl)()!=1||__c1_tg(1,1.0,ff,fd,fl,cf,cd,cl)()!=2||__c1_tg(1,1.0L,ff,fd,fl,cf,cd,cl)()!=3||__c1_tg(1,__c1_i,ff,fd,fl,cf,cd,cl)()!=4||__c1_tg(1,(double _Complex)1,ff,fd,fl,cf,cd,cl)()!=5||__c1_tg(1,(long double _Complex)1,ff,fd,fl,cf,cd,cl)()!=6;}'
    promotion = 'int main(void){return __c1_tg(2,1.0f,1,ff,fd,fl,cf,cd,cl)()!=2||__c1_tg(2,__c1_i,1,ff,fd,fl,cf,cd,cl)()!=5||__c1_tg(3,1.0L,1,__c1_i,ff,fd,fl,cf,cd,cl)()!=6;}'
    unevaluated = 'int main(void){int n=0;volatile float f=1;return __c1_tg(2,n++,f++,ff,fd,fl,cf,cd,cl)()!=2||n!=0||f!=1;}'
    unselected = 'int main(void){return __c1_tg(1,1,missing,fd,also_missing,0,0,0)()!=2;}'
    qualified = 'int main(void){const volatile long double f=1;const float _Complex c=1;return __c1_tg(1,f,ff,fd,fl,cf,cd,cl)()!=3||__c1_tg(1,c,ff,fd,fl,cf,cd,cl)()!=4;}'
    discardedcall = 'float absent(void);int main(void){return __c1_tg(1,absent(),ff,fd,fl,cf,cd,cl)()!=1;}'
    compound = 'int main(void){int n=0;return __c1_tg(1,*(float[]){n++},ff,fd,fl,cf,cd,cl)()!=1||n!=0;}'
}
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $tag + $positive[$n]
}
$negative = [ordered]@{
    pointer = 'int main(void){return __c1_tg(1,(int*)0,ff,fd,fl,cf,cd,cl)();}'
    record = 'struct S{int x;};int main(void){struct S s;return __c1_tg(1,s,ff,fd,fl,cf,cd,cl)();}'
    domain = 'int main(void){return __c1_tg(1,__c1_i,ff,fd,fl,0,0,0)();}'
    missing = 'int main(void){return __c1_tg(1,1,ff,missing,fl,cf,cd,cl)();}'
    arity = 'int main(void){return __c1_tg(0,ff,fd,fl,cf,cd,cl)();}'
}
foreach ($n in @($negative.Keys)) {
    $negative[$n] = $tag + $negative[$n]
}
