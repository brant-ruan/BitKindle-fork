$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $prefix = "#include <stdarg.h>" + [char]10
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $cases = @{
            'sum' = 'int sum(int n,...){va_list p;int s=0;va_start(p,n);while(n--)s+=va_arg(p,int);va_end(p);return s;}int main(void){return sum(0)!=0||sum(1,13)!=13||sum(3,2,3,4)!=9||sum(8,1,2,3,4,5,6,7,8)!=36;}'
            'promotions' = 'enum E{E=-13};int f(int n,...){va_list p;int s=0;va_start(p,n);while(n--)s+=va_arg(p,int);va_end(p);return s;}int main(void){signed char a=-7;unsigned char b=255;short c=-1000;unsigned short d=65535;_Bool e=1;return f(6,a,b,c,d,e,E)!=64771;}'
            'widths' = 'int f(int n,...){va_list p;int r;va_start(p,n);r=va_arg(p,unsigned int)!=4294967295U;r|=va_arg(p,long)!=(-2147483647L-1);r|=va_arg(p,unsigned long)!=4294967295UL;r|=va_arg(p,long long)!=(-9223372036854775807LL-1);r|=va_arg(p,unsigned long long)!=18446744073709551615ULL;va_end(p);return r;}int main(void){return f(0,4294967295U,(-2147483647L-1),4294967295UL,(-9223372036854775807LL-1),18446744073709551615ULL);}'
            'pointers' = 'int f(int n,...){va_list p;int *a;const char *b;void *c;va_start(p,n);a=va_arg(p,int *);b=va_arg(p,const char *);c=va_arg(p,void *);va_end(p);return *a!=29||b[2]!=99||c!=0;}int main(void){int a=29;return f(0,&a,"abc",(void*)0);}'
            'copy' = 'int f(int n,...){va_list a,b;int x,y,z;va_start(a,n);x=va_arg(a,int);va_copy(b,a);y=va_arg(a,int);va_end(a);z=va_arg(b,int);if(va_arg(b,int)!=30)return 1;va_end(b);return x!=10||y!=20||z!=20;}int main(void){return f(0,10,20,30);}'
            'restart' = 'int f(int n,...){va_list p;int a,b;va_start(p,n);a=va_arg(p,int);va_end(p);n=999;va_start(p,n);b=va_arg(p,int);va_end(p);return a!=17||b!=17;}int main(void){return f(0,17);}'
            'helper' = 'int take(va_list *p){return va_arg(*p,int);}int f(int n,...){va_list p;int a,b;va_start(p,n);a=take(&p);b=take(&p);va_end(p);return a!=17||b!=31;}int main(void){return f(0,17,31);}'
            'value-helper' = 'int take(va_list p){int a=va_arg(p,int);return a+va_arg(p,int);}int f(int n,...){va_list p;int r;va_start(p,n);r=take(p);va_end(p);return r;}int main(void){return f(0,17,31)!=48;}'
            'side-effects' = 'int calls;va_list *pick(va_list *p){calls++;return p;}int f(int n,...){va_list p;int a,b;va_start(p,n);a=va_arg(*pick(&p),int);b=va_arg(*pick(&p),int);va_end(p);return a!=17||b!=31||calls!=2;}int main(void){return f(0,17,31);}'
            'function-pointer' = 'int f(int n,...){va_list p;int a;va_start(p,n);a=va_arg(p,int);va_end(p);return n+a;}typedef int (*F)(int,...);F choose(void){return f;}int main(void){F p=f;return p(3,19)!=22||choose()(7,31)!=38;}'
            'prototype' = 'int f(int,...);int f(int n,...){va_list p;int a;va_start(p,n);a=va_arg(p,int);va_end(p);return a;}int main(void){return f(0,77)!=77;}'
            'block-prototype' = 'int main(void){int f(int,...);return f(0,42)!=42;}int f(int n,...){va_list p;int a;va_start(p,n);a=va_arg(p,int);va_end(p);return a;}'
            'complex-definition' = 'int (f)(int n,...){va_list p;int a;va_start(p,n);a=va_arg(p,int);va_end(p);return a;}int main(void){return f(0,73)!=73;}'
            'const-last' = 'int f(const int n,...){va_list p;int a;va_start(p,n);a=va_arg(p,int);va_end(p);return a;}int main(void){return f(0,73)!=73;}'
            'small-records' = 'struct A{char a;};struct B{short a;};struct C{int a;};struct D{int a,b;};int f(int n,...){va_list p;struct A a;struct B b;struct C c;struct D d;va_start(p,n);a=va_arg(p,struct A);b=va_arg(p,struct B);c=va_arg(p,struct C);d=va_arg(p,struct D);va_end(p);return a.a!=-5||b.a!=-300||c.a!=100000||d.a!=17||d.b!=23;}int main(void){struct A a={-5};struct B b={-300};struct C c={100000};struct D d={17,23};return f(0,a,b,c,d);}'
            'large-record' = 'struct S{long long a,b,c;};int f(int n,...){va_list p;struct S a,b;va_start(p,n);a=va_arg(p,struct S);b=va_arg(p,struct S);a.a=999;va_end(p);return a.b!=22||b.a!=11||b.c!=33;}int main(void){struct S s={11,22,33};return f(0,s,s)||s.a!=11;}'
            'odd-record' = 'struct S{char a[3];};int f(int n,...){va_list p;struct S s;va_start(p,n);s=va_arg(p,struct S);va_end(p);return s.a[0]!=11||s.a[1]!=22||s.a[2]!=33;}int main(void){struct S s={{11,22,33}};return f(0,s);}'
            'hidden-return' = 'struct S{long long a,b,c;};struct S f(int n,...){va_list p;struct S s;va_start(p,n);s.a=va_arg(p,int);s.b=va_arg(p,long long);s.c=va_arg(p,int);va_end(p);return s;}int main(void){struct S s=f(0,11,22LL,33);return s.a!=11||s.b!=22||s.c!=33;}'
            'named-record' = 'struct S{long long a,b,c;};int f(struct S s,int n,...){va_list p;int a,b;va_start(p,n);a=va_arg(p,int);b=va_arg(p,int);va_end(p);return s.b!=22||a!=71||b!=81;}int main(void){struct S s={11,22,33};return f(s,0,71,81);}'
            'last-record' = 'struct S{long long a,b,c;};int f(struct S s,...){va_list p;int a;va_start(p,s);a=va_arg(p,int);va_end(p);return s.b!=22||a!=71;}int main(void){struct S s={11,22,33};return f(s,71);}'
            'recursive' = 'int f(int n,...){va_list p;int a,b;va_start(p,n);a=va_arg(p,int);if(n)a+=f(n-1,2,3,4,5);b=va_arg(p,int);va_end(p);return a+b;}int main(void){return f(9,2,3)!=50;}'
            'sizeof' = 'int f(int n,...){va_list p;int a;va_start(p,n);if(sizeof(va_arg(p,int))!=4)return 1;a=va_arg(p,int);va_end(p);return a!=71;}int main(void){return f(0,71);}'
            'function-argument' = 'typedef int (*F)(int);int inc(int a){return a+1;}int f(int n,...){va_list p;F a;va_start(p,n);a=va_arg(p,F);va_end(p);return a(29);}int main(void){return f(0,inc)!=30;}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "varargs-$mode-$name" ($prefix + $cases[$name])
        }
        $extra = @{
            'postfix' = 'struct S{int a,b;};typedef int (*F)(int);int inc(int a){return a+1;}int f(int n,...){va_list p;int r;va_start(p,n);r=va_arg(p,struct S).b!=22;r|=va_arg(p,char *)[2]!=99;r|=va_arg(p,F)(30)!=31;va_end(p);return r;}int main(void){struct S s={11,22};return f(0,s,"abc",inc);}'
            'union' = 'union U{unsigned long long a;char b[8];};int f(int n,...){va_list p;union U u;va_start(p,n);u=va_arg(p,union U);va_end(p);return u.a!=123456789012345ULL;}int main(void){union U u={123456789012345ULL};return f(0,u);}'
            'named-small-record' = 'struct S{int a,b;};int f(struct S s,...){va_list p;int r;va_start(p,s);r=va_arg(p,int);va_end(p);return s.a!=11||s.b!=22||r!=73;}int main(void){struct S s={11,22};return f(s,73);}'
            'hidden-stack-return' = 'struct S{long long a,b,c;};struct S f(int a,int b,int n,...){va_list p;struct S s;va_start(p,n);s.a=va_arg(p,int);s.b=va_arg(p,int);s.c=va_arg(p,int);va_end(p);return s;}int main(void){struct S s=f(1,2,3,11,22,33);return s.a!=11||s.b!=22||s.c!=33;}'
            'return-function-pointer' = 'typedef int (*F)(int);int inc(int a){return a+1;}F f(int n,...){va_list p;F r;va_start(p,n);r=va_arg(p,F);va_end(p);return r;}int main(void){return f(0,inc)(31)!=32;}'
            'unused-tail' = 'int f(int n,...){return n;}int main(void){return f(0)||f(0,1,2,3,4,5,6,7);}'
            'call-evaluation' = 'int g;int next(void){return ++g;}int f(int n,...){va_list p;int sum=0;va_start(p,n);while(n--)sum+=va_arg(p,int);va_end(p);return sum;}int main(void){int r=f(7,next(),next(),next(),next(),next(),next(),next());return r!=28||g!=7;}'
            'fixed-conversion' = 'int f(unsigned char a,int n,...){va_list p;int r;va_start(p,n);r=va_arg(p,int);va_end(p);return a!=1||r!=73;}int main(void){return f(257,0,73);}'
            'list-array' = 'int f(int n,...){va_list p[2];int a,b;va_start(p[0],n);va_copy(p[1],p[0]);a=va_arg(p[0],int);b=va_arg(p[1],int);va_end(p[0]);va_end(p[1]);return a!=73||b!=73;}int main(void){return f(0,73);}'
        }
        foreach ($name in ($extra.Keys | Sort-Object)) {
            Check "varargs-$mode-$name" ($prefix + $extra[$name])
        }
        foreach ($fixed in 1, 2, 3, 4, 5, 8, 16, 126) {
            $params = (1..$fixed | ForEach-Object { "int a$_" }) -join ','
            $args = (1..$fixed) -join ','
            $body = "int f($params,...){va_list p;int v;va_start(p,a$fixed);v=va_arg(p,int);va_end(p);return v!=173;}int main(void){return f($args,173);}"
            Check "varargs-$mode-fixed-$fixed" ($prefix + $body)
        }
        $rejects = @{
            'no-fixed' = 'int f(...);int main(void){return 0;}'
            'void-fixed' = 'int f(void,...);int main(void){return 0;}'
            'after-ellipsis' = 'int f(int,...,int);int main(void){return 0;}'
            'trailing-comma' = 'int f(int,...,);int main(void){return 0;}'
            'too-few' = 'int f(int a,int b,...){return a;}int main(void){return f(1);}'
            'fixed-mismatch' = 'int f(int,...);int f(int);int main(void){return 0;}'
            'unspecified-first' = 'int f();int f(int,...);int main(void){return 0;}'
            'unspecified-last' = 'int f(int,...);int f();int main(void){return 0;}'
            'old-definition' = 'int f(int,...);int f(a)int a;{return a;}int main(void){return 0;}'
            'pointer-mismatch' = 'int f(int a,...){return a;}int main(void){int (*p)(int)=f;return p(1);}'
            'outside' = 'int main(void){va_list p;va_start(p,x);return 0;}'
            'ordinary' = 'int f(int n){va_list p;va_start(p,n);return 0;}int main(void){return f(0);}'
            'wrong-last' = 'int f(int a,int b,...){va_list p;va_start(p,a);return 0;}int main(void){return f(1,2);}'
            'shadow-last' = 'int f(int n,...){va_list p;{int n=1;va_start(p,n);}return 0;}int main(void){return f(0);}'
            'register-last' = 'int f(register int n,...){va_list p;va_start(p,n);return 0;}int main(void){return f(0);}'
            'void-arg' = 'int f(int n,...){va_list p;va_start(p,n);va_arg(p,void);return 0;}int main(void){return f(0);}'
            'array-arg' = 'int f(int n,...){va_list p;va_start(p,n);va_arg(p,int[2]);return 0;}int main(void){return f(0);}'
            'function-arg' = 'int f(int n,...){va_list p;va_start(p,n);va_arg(p,int(int));return 0;}int main(void){return f(0);}'
            'const-list' = 'int f(int n,...){va_list const p=0;return va_arg(p,int);}int main(void){return f(0,1);}'
            'changed-fixed-count' = 'int f(int,int,...);int f(int n,...){return n;}int main(void){return 0;}'
            'changed-fixed-type' = 'int f(char *,...);int f(int n,...){return n;}int main(void){return 0;}'
            'unprototyped-indirect' = 'int f(int a,...){return a;}int main(void){int (*p)()=f;return 0;}'
            'ellipsis-old-list' = 'int f(a,...)int a;{return a;}int main(void){return 0;}'
            'incomplete-arg' = 'struct S;int f(int n,...){va_list p;va_start(p,n);va_arg(p,struct S);return 0;}int main(void){return f(0);}'
            'record-not-lvalue' = 'struct S{int a;};int f(int n,...){va_list p;va_start(p,n);va_arg(p,struct S).a=9;return 0;}int main(void){struct S s={1};return f(0,s);}'
        }
        foreach ($name in ($rejects.Keys | Sort-Object)) {
            Check "varargs-$mode-reject-$name" ($prefix + $rejects[$name]) 0 $true
        }
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
