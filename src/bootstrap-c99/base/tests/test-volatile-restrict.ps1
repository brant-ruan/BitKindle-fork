$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    $stdarg = '#include "' + ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include/stdarg.h')).Replace('\', '/')) + '"' + [char]10
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $cases = @{
            'scalar' = 'volatile int v;int main(void){volatile int a=7;v=a;a+=3;v++;return a!=10||v!=8;}'
            'mixed-qualifiers' = 'int main(void){int x=7;const volatile int *p=&x;volatile const int *q=p;return *q!=7;}'
            'repeat-qualifiers' = 'typedef volatile int V;volatile V x;int main(void){volatile volatile const const int y=13;x=7;return x!=7||y!=13;}'
            'pointer-object' = 'int main(void){int a=11,b=23;int *volatile p=&a;int *volatile *q=&p;*q=&b;return **q!=23;}'
            'pointer-layers' = 'int main(void){int a=11;int *p=&a;int *volatile *q=&p;int *const volatile *r=q;return **r!=11;}'
            'void-pointer' = 'int main(void){int a=11;volatile void *v=&a;const volatile void *c=v;volatile int *p=v;return *p!=11||c!=v;}'
            'conditional-qualifiers' = 'int main(void){int a=11;const int *p=&a;volatile int *q=&a;const volatile int *r=1?p:q;return *r!=11||p!=q;}'
            'struct-members' = 'struct S{int a;int b[2];};int main(void){volatile struct S s={11,{22,33}};volatile int *p=&s.b[1];s.a+=2;return s.a!=13||*p!=33;}'
            'member-volatile' = 'struct S{volatile int a;int b;};int main(void){struct S s={11,22},t;t=s;t.a++;return s.a!=11||t.a!=12||t.b!=22;}'
            'aggregate-assignment-chain' = 'struct S{int a,b,c;};volatile struct S a,b;int main(void){struct S c={11,22,33},d;d=(a=b=c);return d.a!=11||a.b!=22||b.c!=33;}'
            'aggregate-self-assignment' = 'struct S{int a,b,c;};volatile struct S a={11,22,33};int main(void){a=a;return a.a!=11||a.b!=22||a.c!=33;}'
            'aggregate-return' = 'struct S{int a,b,c;};volatile struct S a={11,22,33};struct S f(void){return a;}int main(void){struct S b=f();return b.a!=11||b.b!=22||b.c!=33;}'
            'aggregate-small-return' = 'struct S{int a,b;};volatile struct S a={11,22};struct S f(void){return a;}int main(void){struct S b=f();return b.a!=11||b.b!=22;}'
            'aggregate-argument' = 'struct S{int a,b,c;};int f(struct S s){return s.a+s.b+s.c;}int main(void){volatile struct S s={11,22,33};return f(s)!=66;}'
            'aggregate-conditional' = 'struct S{int a,b;};volatile struct S a={11,22},b={33,44};int main(void){struct S c=1?a:b,d=0?a:b;return c.a!=11||c.b!=22||d.a!=33||d.b!=44;}'
            'aggregate-member-assignment-result' = 'struct S{int a,b;};volatile struct S a={11,22};int main(void){struct S b;return (b=a).a!=11||(a=b).b!=22;}'
            'union' = 'union U{int a;char b[4];};int main(void){volatile union U a={123};union U b=a;return b.a!=123;}'
            'array-typedef' = 'typedef int A[2][2];volatile A a={{1,2},{3,4}};int main(void){volatile int (*p)[2]=a;p[1][0]=9;return a[0][1]!=2||a[1][0]!=9;}'
            'inferred-array' = 'int main(void){volatile int a[]={1,2,3};return sizeof(a)!=12||a[2]!=3;}'
            'inferred-record-array' = 'struct S{volatile int a;int b;};int main(void){struct S a[]={{1,2},{3,4}};return sizeof(a)!=16||a[1].b!=4;}'
            'array-partial-init' = 'int main(void){volatile int a[9]={11,22};int i;for(i=2;i<9;i++)if(a[i])return 1;return a[0]!=11||a[1]!=22;}'
            'volatile-string-array' = 'int main(void){volatile char a[8]="abc";return a[0]!=97||a[3]||a[7];}'
            'parameter' = 'int f(volatile int a){a++;return a;}int f(int);int main(void){return f(7)!=8;}'
            'old-parameter' = 'int f(a)volatile int a;{a++;return a;}int main(void){return f(7)!=8;}'
            'qualified-return' = 'volatile int f(void){return 7;}int main(void){return f()!=7;}'
            'register-volatile' = 'int main(void){register volatile int a=7;a++;return a!=8;}'
            'static-local' = 'int f(void){static volatile int a=0;return ++a;}int main(void){return f()!=1||f()!=2;}'
            'extern-redeclaration' = 'extern volatile int a;volatile int a=7;int main(void){extern volatile int a;return a!=7;}'
            'address-sizeof-no-read' = 'int main(void){volatile int *p=(volatile int*)1;volatile int *q=&*p;return q!=p||sizeof(*p)!=4;}'
            'short-circuit-no-read' = 'int main(void){volatile int *p=(volatile int*)1;return (0&&*p)||(1||*p)!=1||(0?*p:3)!=3;}'
            'lhs-side-effects' = 'volatile int a[2];int n;volatile int *next(void){return &a[n++];}int main(void){*next()=11;*next()=22;return n!=2||a[0]!=11||a[1]!=22;}'
            'pointer-increment' = 'int main(void){volatile int a[2]={11,22};volatile int *volatile p=a;*p++=33;return p!=a+1||a[0]!=33||*p!=22;}'
            'loop-reloads' = 'volatile int v;void next(void){v++;}int main(void){int n=0;while(v<9){next();n++;}return n!=9;}'
            'sequence-points' = 'volatile int v;int f(void){return v;}int main(void){v=7;return (v=9,f())!=9||((v=11)&&f()!=11)||((v=13)?f()!=13:1);}'
            'restrict-local' = 'int main(void){int a[3]={1,2,3};int *restrict p=a;p[1]+=7;return p[1]!=9;}'
            'restrict-parameters' = 'void copy(int *restrict a,const int *restrict b,int n){int i;for(i=0;i<n;i++)a[i]=b[i];}int main(void){int a[3],b[3]={1,2,3};copy(a,b,3);return a[0]!=1||a[2]!=3;}'
            'restrict-prototype' = 'int f(int *restrict);int f(int *p){return *p;}int main(void){int a=11;return f(&a)!=11;}'
            'restrict-typedef' = 'typedef int *P;int main(void){int a=11;restrict P p=&a;return *p!=11;}'
            'restrict-array-typedef' = 'typedef int *A[2];int main(void){int a=11,b=22;restrict A p={&a,&b};return *p[0]!=11||*p[1]!=22;}'
            'restrict-void' = 'void *restrict p;int main(void){int a=11;p=&a;return *(int*)p!=11;}'
            'restrict-incomplete' = 'struct S;struct S *restrict p;struct S{int a;};int main(void){struct S s={11};p=&s;return p->a!=11;}'
            'restrict-function-pointer-object' = 'typedef int (*F)(int);int f(int a){return a+1;}int main(void){F q=f;F *restrict p=&q;return (*p)(11)!=12;}'
            'restrict-qualified-pointer' = 'int main(void){int a=11;int *restrict p=&a;int *restrict *q=&p;int *const volatile restrict *r=q;return **r!=11;}'
            'all-three' = 'int main(void){int a=11;int *const volatile restrict p=&a;*p=22;return *p!=22;}'
            'restrict-nested-scope' = 'int main(void){int a=11;int *restrict p=&a;{int *restrict q=p;*q=22;}return *p!=22;}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "vr-$mode-$name" $cases[$name]
        }
        $vaCases = @{
            'volatile-list' = 'int f(int n,...){volatile va_list p;int a,b;va_start(p,n);a=va_arg(p,int);b=va_arg(p,int);va_end(p);return a!=17||b!=31;}int main(void){return f(0,17,31);}'
            'volatile-list-copy' = 'int f(int n,...){volatile va_list p;va_list q;int a,b;va_start(p,n);va_copy(q,p);a=va_arg(p,int);b=va_arg(q,int);va_end(p);va_end(q);return a!=17||b!=17;}int main(void){return f(0,17);}'
            'volatile-list-helper' = 'int take(volatile va_list *p){return va_arg(*p,int);}int f(int n,...){volatile va_list p;int a,b;va_start(p,n);a=take(&p);b=take(&p);va_end(p);return a!=17||b!=31;}int main(void){return f(0,17,31);}'
            'restrict-list' = 'int f(int n,...){va_list restrict p;int a;va_start(p,n);a=va_arg(p,int);va_end(p);return a!=17;}int main(void){return f(0,17);}'
        }
        foreach ($name in ($vaCases.Keys | Sort-Object)) {
            Check "vr-$mode-$name" ($stdarg + $vaCases[$name])
        }
        $rejects = @{
            'discard-volatile' = 'int main(void){volatile int a=1;int *p=&a;return 0;}'
            'discard-via-void' = 'int main(void){volatile int a=1;void *p=&a;return 0;}'
            'deep-qualification' = 'int main(void){int a=1;int *p=&a;volatile int **q=&p;return 0;}'
            'redeclaration' = 'extern volatile int a;int a;int main(void){return 0;}'
            'pointer-redeclaration' = 'extern int *volatile a;int *a;int main(void){return 0;}'
            'const-volatile-store' = 'int main(void){const volatile int a=1;a=2;return 0;}'
            'const-volatile-increment' = 'int main(void){const volatile int a=1;a++;return 0;}'
            'const-volatile-record' = 'struct S{int a;};int main(void){const volatile struct S a={1};a.a=2;return 0;}'
            'volatile-function' = 'typedef int F(void);volatile F f;int main(void){return 0;}'
            'restrict-int' = 'restrict int a;int main(void){return 0;}'
            'restrict-void-type' = 'restrict void *p;int main(void){return 0;}'
            'restrict-record' = 'struct S{int a;};restrict struct S a;int main(void){return 0;}'
            'restrict-array' = 'typedef int A[2];restrict A a;int main(void){return 0;}'
            'restrict-function' = 'int (*restrict p)(void);int main(void){return 0;}'
            'restrict-function-typedef' = 'typedef int (*F)(void);restrict F p;int main(void){return 0;}'
            'restrict-redeclaration' = 'int *restrict p;extern int *p;int main(void){return 0;}'
            'discard-restrict-pointer-target' = 'int main(void){int a=1;int *restrict p=&a;int **q=&p;return 0;}'
            'discard-member-volatile' = 'struct S{int a;};int main(void){volatile struct S s={1};int *p=&s.a;return 0;}'
            'discard-array-volatile' = 'int main(void){volatile int a[2];int *p=a;return 0;}'
            'parameter-pointee-mismatch' = 'int f(volatile int *);int f(int *p){return *p;}int main(void){return 0;}'
        }
        foreach ($name in ($rejects.Keys | Sort-Object)) {
            Check "vr-$mode-reject-$name" $rejects[$name] 0 $true
        }
        # Runtime inspection reads actual generated instructions. It does not create or patch code.
        $scanner = @'
int refs(unsigned char *p,unsigned char *end,void *target,int write){
    int n=0,k,len,op;unsigned int d;unsigned long long a;
    while(p<end){
        k=0;if(p[k]==72||p[k]==102)k++;
        op=p[k++];
        if(op==15){op=p[k++];if(write||!(op==182||op==183||op==190||op==191)){p++;continue;}}
        else if(write?!(op==137||op==136):op!=139){p++;continue;}
        if(p[k++]!=5){p++;continue;}len=k+4;
        d=(unsigned int)p[k]|((unsigned int)p[k+1]<<8)|((unsigned int)p[k+2]<<16)|((unsigned int)p[k+3]<<24);
        a=(unsigned long long)(p+len)+(long long)(int)d;
        if(a==(unsigned long long)target)n++;
        p+=len;
    }
    return n;
}
'@
        $accesses = @(
            @('store', 'v=7;', 0, 1),
            @('read', 'v;', 1, 0),
            @('address', '&v;', 0, 0),
            @('size', 'sizeof(v);', 0, 0),
            @('void-read', '(void)v;', 1, 0),
            @('compound', 'v+=7;', 1, 1),
            @('increment', 'v++;', 1, 1),
            @('self', 'v=v;', 1, 1),
            @('two-reads', 'v+v;', 2, 0),
            @('two-stores', 'v=7;v=9;', 0, 2),
            @('assignment-result', '(v=7)+1;', 0, 1),
            @('parenthesized-lhs', '(v)=7;', 0, 1)
        )
        foreach ($type in 'char', 'short', 'int', 'long long') {
            foreach ($item in $accesses) {
                $src = "volatile $type v;int f(void){" + $item[1] + "return 0;}int end(void){return 0;}" + $scanner +
                "int main(void){f();return refs((unsigned char*)f,(unsigned char*)end,(void*)&v,0)!=" + $item[2] +
                "||refs((unsigned char*)f,(unsigned char*)end,(void*)&v,1)!=" + $item[3] + ";}"
                Check ("vr-$mode-access-" + $type.Replace(' ', '-') + '-' + $item[0]) $src
            }
        }
        $indirectScanner = @'
int refs(unsigned char *p,unsigned char *end,int write){
    int n=0,k,op;
    while(p<end){
        k=0;if(p[k]==72||p[k]==102)k++;
        op=p[k++];
        if(op==15){op=p[k++];if(write||!(op==182||op==183||op==190||op==191)){p++;continue;}}
        else if(write?!(op==137||op==136):op!=139){p++;continue;}
        if(p[k]!=(write?1:0)){p++;continue;}
        n++;p+=k+1;
    }
    return n;
}
'@
        foreach ($type in 'char', 'short', 'int', 'long long') {
            foreach ($item in $accesses) {
                $body = [regex]::Replace($item[1], '\bv\b', '(*p)')
                $src = "volatile $type v;int f(volatile $type *p){" + $body + "return 0;}int end(void){return 0;}" + $indirectScanner +
                "int main(void){f(&v);return refs((unsigned char*)f,(unsigned char*)end,0)!=" + $item[2] +
                "||refs((unsigned char*)f,(unsigned char*)end,1)!=" + $item[3] + ";}"
                Check ("vr-$mode-indirect-access-" + $type.Replace(' ', '-') + '-' + $item[0]) $src
            }
        }
        $copies = @(
            @('discard', 's;', 1), @('void', '(void)s;', 1), @('address', '&s;', 0), @('size', 'sizeof(s);', 0),
            @('comma', 's,s;', 2), @('member', 's.a;', 0), @('store', 's=t;', 1),
            @('copy', 't=s;', 2), @('chain', 't=s=t;', 2), @('self', 's=s;', 2)
        )
        foreach ($item in $copies) {
            $src = 'struct S{int a,b,c;};volatile struct S s;struct S t;int f(void){' + $item[1] +
            'return 0;}int end(void){return 0;}int main(void){unsigned char *p=(unsigned char*)f,*e=(unsigned char*)end;int n=0;f();while(p+1<e){if(p[0]==243&&p[1]==164)n++;p++;}return n!=' + $item[2] + ';}'
            Check ("vr-$mode-record-access-" + $item[0]) $src
        }
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
