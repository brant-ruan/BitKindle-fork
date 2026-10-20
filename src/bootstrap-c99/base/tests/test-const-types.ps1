foreach ($mode in 'c89', 'c99') {
    $savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
    try {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $positive = @{
            'scalar' = 'const int g=7;int main(void){int const a=3;const unsigned long long b=4294967297ULL;const signed char c=-3;const unsigned short d=65535;return g==7&&a+2==5&&b+1==4294967298ULL&&c==-3&&d==65535&&sizeof(a)==4;}'
            'pointer' = 'int main(void){int n=3;const int *p=&n;int *const q=&n;*q=7;return *p==7&&p==q&&sizeof(q)==8;}'
            'nested-pointer' = 'int main(void){int n=9;int *p=&n;int *const *q=&p;int **const r=&p;**r=11;return **q==11;}'
            'typedef' = 'typedef int *P;typedef const int CI;typedef CI CC;int main(void){int n=3;const P p=&n;const CC q=7;*p=4;return *p==4&&q==7;}'
            'array-typedef' = 'typedef int A[2][3];const A a={{1,2,3},{4,5,6}};int main(void){const int (*p)[3]=a;return p[1][2]==6&&sizeof(a)==24;} '
            'string-arrays' = 'const char a[]="ab";int main(void){const char b[4]="xyz";return sizeof(a)==3&&a[1]==98&&b[2]==122&&b[3]==0;}'
            'array-inference' = 'const int a[]={1,2,3};int main(void){const int b[]={4,5};return sizeof(a)==12&&sizeof(b)==8&&b[1]+a[2]==8;}'
            'record-member' = 'struct S{int n;int a[2];};int main(void){const struct S s={7,{8,9}};const struct S *p=&s;const int *q=&s.a[1];return s.n==7&&p->a[0]==8&&*q==9;}'
            'const-member-init' = 'struct S{const int n;int v;};int main(void){struct S a={3,4};struct S b=a;b.v=5;return b.n==3&&b.v==5;}'
            'record-copy' = 'struct S{int n;long long v;};int main(void){const struct S a={3,4294967297LL};struct S b=a;const struct S c=b;b.n=4;return c.n==3&&b.n==4&&c.v==4294967297LL;}'
            'record-parameter' = 'struct S{int n;long long v;};int f(const struct S s){return s.n+(int)s.v;}int main(void){const struct S s={3,4};return f(s)==7;}'
            'prototype' = 'int f(int);int f(const int x){return x+1;}int main(void){int (*p)(const int)=f;return p(3)==4;}'
            'complex-definition' = 'int (f)(const int x){return x+1;}int main(void){int (*const p)(int)=f;return p(4)==5;}'
            'pointer-param' = 'int f(const int *const p){return *p;}int main(void){int n=9;return f(&n)==9;}'
            'oldstyle' = 'int f(int);int f(x) const int x;{return x+2;}int main(void){return f(3)==5;}'
            'bool-parameter' = 'int f(const _Bool b){return b;}int main(void){return f(256)==1;}'
            'pointer-return' = 'const int n=9;const int *f(void){return &n;}int main(void){return *f()==9;}'
            'conditional' = 'int main(void){int n=3;int *p=&n;const int *q=&n;const int *r=1?p:q;const void *v=0?(void*)p:q;return *r==3&&v==q;} '
            'void-conversion' = 'int main(void){int n=5;void *p=&n;const void *q=p;const int *r=q;return *r==5&&q==r;} '
            'explicit-cast' = 'int main(void){int n=3;const int *p=&n;int *q=(int *)p;*q=7;return *p==7;} '
            'pointer-difference' = 'int main(void){int a[4];const int *p=a;return p+3-a==3&&a+2-p==2&&p<a+3&&a+1>p;} '
            'redeclarations' = 'extern const int n;const int n=7;extern int const n;int main(void){return n==7;}'
            'repeated' = 'typedef const int T;const const T x=9;int main(void){return x==9;} '
            'constant-expression' = 'enum {N=(const int)3};int a[(const unsigned short)65537];int main(void){switch(N){case (const int)3:return sizeof(a)==4;}return 0;} '
            'static-member-address' = 'struct S{int n;int a[2];};const struct S s={3,{4,5}};const int *p=&s.n;const int *q=&s.a[1];int main(void){return *p==3&&*q==5;} '
            'parameter-array' = 'int f(const int a[2]){return a[1];}int main(void){int a[2]={3,7};return f(a)==7;}'
            'const-pointer-array' = 'int n=3,m=4;int *const p[]={&n,&m};int main(void){*p[0]=8;return n==8&&*p[1]==4;} '
            'const-incomplete' = 'struct S;const struct S *p;struct S{int n;};int main(void){struct S s={3};p=&s;return p->n==3;} '
            'const-enum' = 'enum E{A=3};int main(void){const enum E e=A;return e==3&&sizeof(e)==4;} '
            'function-pointer-const' = 'int f(int n){return n+3;}int main(void){int (*const p)(int)=f;return (*p)(4)==7;} '
        }
        $positive += @{
            'qualifier-placement' = 'const static int a=3;static int const b=4;const c=5;int main(void){return a+b+c==12;}'
            'qualified-void-return' = 'const void f(void){return;}int main(void){f();return 1;}'
            'qualified-value-return' = 'const int f(void){return 257;}int main(void){return f()==257&&sizeof(f())==4;}'
            'prototype-definition-strip' = 'int f(const int);int f(int n){n++;return n;}int main(void){return f(3)==4;}'
            'pointer-prototype-strip' = 'int f(int *const);int f(int *p){p++;return *p;}int main(void){int a[]={3,4};return f(a)==4;}'
            'nested-qualification' = 'struct A{int v;};struct B{struct A a;};int main(void){const struct B b={{7}};const int *p=&b.a.v;return *p==7;}'
            'union-qualification' = 'union U{int a[2];long long v;};int main(void){const union U u={{3,4}};return u.a[1]==4;}'
            'const-flexible' = 'struct S{long long n;char c;char data[];};int main(void){struct S s={3,4};const struct S *p=&s;s.data[0]=7;return p->data[0]==7&&sizeof(*p)==16;}'
            'initializer-checkpoint' = 'struct S{int n;};int main(void){const struct S a[]={{3},{4}};const int b[]={sizeof(struct T{const int x;}),sizeof(const struct S)};struct T t={9};return a[1].n==4&&sizeof(a)==8&&b[0]==4&&b[1]==4&&t.x==9;}'
            'nested-declaration-state' = 'const struct S{int n;int *p;} s={3,0};int main(void){struct S t={4,0};t.n=5;return s.n==3&&t.n==5;}'
            'record-const-pointer-member' = 'struct S{int *const p;};int main(void){int n=3;struct S s={&n};*s.p=7;return n==7;}'
            'conditional-qualification' = 'int main(void){int n=3;const int *p=&n;void *q=&n;const void *r=1?p:q;return r==p;}'
            'qualified-null' = 'int main(void){const int *p=0;const void *q=(void *)0;return p==q&&q==0;}'
            'const-sizeof-address' = 'int main(void){const int x=7;return sizeof(&x)==8&&sizeof(*&x)==4&&*&x==7;}'
            'const-scope' = 'int main(void){int n=3;{const int n=4;if(n!=4)return 0;}n=5;return n==5;}'
        }
        foreach ($name in ($positive.Keys | Sort-Object)) {
            Check "const-$mode-$name" $positive[$name] 1 
        }
        $negative = @{
            'assign' = 'int main(void){const int n=3;n=4;return 0;}'
            'compound' = 'int main(void){const int n=3;n+=4;return 0;}'
            'prefix' = 'int main(void){const int n=3;++n;return 0;}'
            'postfix' = 'int main(void){const int n=3;n--;return 0;}'
            'pointed-store' = 'int main(void){int n=3;const int *p=&n;*p=4;return 0;}'
            'const-pointer-store' = 'int main(void){int n=3;int *const p=&n;p=&n;return 0;}'
            'drop-init' = 'int main(void){const int n=3;int *p=&n;return 0;}'
            'drop-assignment' = 'int main(void){int n=3;const int *p=&n;int *q;q=p;return 0;}'
            'drop-void' = 'int main(void){const int n=3;void *p=&n;return 0;}'
            'drop-from-void' = 'int main(void){const void *p=0;int *q=p;return 0;}'
            'drop-argument' = 'int f(int *p){return *p;}int main(void){const int n=3;return f(&n);}'
            'drop-return' = 'const int n=3;int *f(void){return &n;}int main(void){return 0;}'
            'unsafe-deep' = 'int main(void){int **p=0;const int **q=p;return 0;}'
            'deep-comparison' = 'int main(void){int **p=0;const int **q=0;return p==q;}'
            'deep-conditional' = 'int main(void){int **p=0;const int **q=0;return (1?p:q)==0;}'
            'member' = 'struct S{int n;};int main(void){const struct S s={3};s.n=4;return 0;}'
            'arrow-member' = 'struct S{int n;};int main(void){struct S s={3};const struct S *p=&s;p->n=4;return 0;}'
            'array-member' = 'struct S{int a[2];};int main(void){const struct S s={{3,4}};s.a[1]=7;return 0;}'
            'array-typedef-store' = 'typedef int A[2];int main(void){const A a={3,4};a[0]=5;return 0;}'
            'record-assignment' = 'struct S{const int n;};int main(void){struct S a={3},b={4};a=b;return 0;}'
            'nested-record-assignment' = 'struct S{const int n;};struct T{struct S s[2];};int main(void){struct T a={{{1},{2}}},b={{{3},{4}}};a=b;return 0;}'
            'union-assignment' = 'union U{const int n;int x;};int main(void){union U a={1},b={2};a=b;return 0;}'
            'parameter-store' = 'int f(const int n){n=4;return n;}int main(void){return f(3);}'
            'old-parameter-store' = 'int f(n) const int n;{n=4;return n;}int main(void){return f(3);}'
            'complex-param-store' = 'int (f)(const int n){n++;return n;}int main(void){return f(3);}'
            'redeclare-drop' = 'const int n=3;extern int n;int main(void){return 0;}'
            'redeclare-add' = 'int n=3;extern const int n;int main(void){return 0;}'
            'prototype-pointee' = 'int f(int *);int f(const int *);int main(void){return 0;}'
            'function-typedef' = 'typedef int F(void);const F f;int main(void){return 0;}'
            'void-param' = 'int f(const void);int main(void){return 0;}'
            'const-not-ice' = 'const int n=3;int a[n];int main(void){return 0;}'
            'static-drop-member' = 'struct S{int n;};const struct S s={3};int *p=&s.n;int main(void){return 0;}'
        }
        $negative += @{
            'conditional-drop' = 'int main(void){int n=3;int *p=&n;const int *q=&n;int *r=1?p:q;return 0;}'
            'conditional-void-drop' = 'int main(void){int n=3;void *p=&n;const int *q=&n;void *r=1?p:q;return 0;}'
            'member-pointer-drop' = 'struct S{int *p;};int main(void){int n=3;const struct S s={&n};int **p=&s.p;return 0;}'
            'nested-qualified-store' = 'struct S{int n;};struct T{struct S s;};int main(void){const struct T t={{3}};t.s.n=4;return 0;}'
            'qualified-array-address' = 'int main(void){const int a[2]={3,4};int (*p)[2]=&a;return 0;}'
            'void-object-extern' = 'extern const void x;int main(void){return 0;}'
            'const-pointer-increment' = 'int main(void){int a[2];int *const p=a;p++;return 0;}'
        }
        foreach ($name in ($negative.Keys | Sort-Object)) {
            Check "const-$mode-reject-$name" $negative[$name] 0 $true 
        }
    }
    finally {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process') 
    }
}
