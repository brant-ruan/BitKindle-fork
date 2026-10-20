$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $cases = @{
            'basic' = 'struct S{unsigned a:3,b:5;int c:7;};int main(void){struct S s={5,17,-33};return sizeof(s)!=4||s.a!=5||s.b!=17||s.c!=-33;}'
            'plain-int-signed' = 'struct S{int a:3;};int main(void){struct S s={-4};if(s.a!=-4)return 1;s.a=3;return s.a!=3;}'
            'bool' = 'struct S{_Bool a:1,b:1;};int main(void){struct S s={3,0};if(sizeof(s)!=1||s.a!=1||s.b!=0)return 1;s.b=-7;return s.b!=1;}'
            'bool-increments' = 'struct S{_Bool a:1;};int main(void){struct S s={0};if(s.a++!=0||s.a!=1)return 1;if(++s.a!=1||s.a!=1)return 2;if(s.a--!=1||s.a!=0)return 3;return --s.a!=1||s.a!=1;}'
            'same-unit' = 'struct S{unsigned a:12;int b:12;unsigned c:8;};int main(void){struct S s={0xabc,-1,0xde};unsigned char *p=(unsigned char*)&s;return sizeof(s)!=4||p[0]!=0xbc||p[1]!=0xfa||p[2]!=0xff||p[3]!=0xde;}'
            'cross-unit' = 'struct S{unsigned a:31,b:2;char c;};int main(void){struct S s={7,3,11};return sizeof(s)!=12||__offsetof(struct S,c)!=8||s.a!=7||s.b!=3||s.c!=11;}'
            'ordinary-padding' = 'struct S{char a;unsigned b:3;char c;};int main(void){struct S s={11,7,22};return sizeof(s)!=12||__offsetof(struct S,c)!=8||s.a!=11||s.b!=7||s.c!=22;}'
            'bool-int-units' = 'struct S{_Bool a:1;unsigned b:3;_Bool c:1;char d;};int main(void){struct S s={1,7,1,11};return sizeof(s)!=12||__offsetof(struct S,d)!=9||s.a!=1||s.b!=7||s.c!=1;}'
            'anonymous-init' = 'struct S{unsigned :3;unsigned a:4;unsigned :9;int b:5;};int main(void){struct S s={7,-5};return s.a!=7||s.b!=-5||sizeof(s)!=4;}'
            'zero-separation' = 'struct S{unsigned a:3;unsigned :0;unsigned b:2;};int main(void){struct S s={7,3};unsigned char *p=(unsigned char*)&s;return sizeof(s)!=8||p[0]!=7||p[4]!=3;}'
            'zero-after-ordinary' = 'struct S{char a;unsigned :0;char b;};int main(void){return sizeof(struct S)!=2||__offsetof(struct S,b)!=1;}'
            'zero-after-bool' = 'struct S{_Bool a:1;unsigned :0;char b;};int main(void){return sizeof(struct S)!=8||__offsetof(struct S,b)!=4;}'
            'zero-at-end' = 'struct S{_Bool a:1;unsigned :0;};int main(void){return sizeof(struct S)!=4;}'
            'zero-at-start' = 'struct S{unsigned :0;char a;};int main(void){return sizeof(struct S)!=1;}'
            'consecutive-zero' = 'struct S{_Bool a:1;_Bool :0;unsigned :0;char b;};int main(void){return sizeof(struct S)!=2||__offsetof(struct S,b)!=1;}'
            'union' = 'union U{unsigned a:3;int b:4;};int main(void){union U u={7};if(sizeof(u)!=4||u.a!=7)return 1;u.b=-2;return u.b!=-2||u.a!=6;}'
            'union-unnamed' = 'union U{unsigned :7;_Bool a:1;unsigned b:4;};int main(void){union U u={1};return sizeof(u)!=4||u.a!=1;}'
            'nested' = 'struct I{unsigned a:3;int b:5;};struct O{char x;struct I i;};int main(void){struct O o={11,{7,-9}};o.i.a=3;return o.x!=11||o.i.a!=3||o.i.b!=-9||sizeof(o)!=8;}'
            'global-init' = 'struct S{unsigned :2;unsigned a:5;int b:7;};struct S s={17,-33};int main(void){return s.a!=17||s.b!=-33;}'
            'static-local' = 'struct S{unsigned a:5;};int f(void){static struct S s={17};return s.a++;}int main(void){return f()!=17||f()!=18;}'
            'nested-braces' = 'struct S{unsigned a:5;int b:7;};int main(void){struct S s={{{17,},},{{-33}}};return s.a!=17||s.b!=-33;}'
            'partial-init' = 'struct S{unsigned a:5,b:7,c:8;};int main(void){struct S s={17};return s.a!=17||s.b||s.c;}'
            'inferred-array' = 'struct S{unsigned :2;unsigned a:5;int b:7;};int main(void){struct S s[]={{17,-33},{7,-9}};return sizeof(s)!=8||s[0].a!=17||s[1].b!=-9;}'
            'elided-array' = 'struct S{unsigned :2;unsigned a:5;int b:7;};int main(void){struct S s[]={17,-33,7,-9};return sizeof(s)!=8||s[0].a!=17||s[1].b!=-9;}'
            'sizeof-type-in-array' = 'int main(void){int a[]={sizeof(struct {unsigned x:3;}),sizeof(struct {int x;})};return sizeof(a)!=8||a[0]!=4||a[1]!=4;}'
            'conditional-type-in-array' = 'int main(void){int a[]={1?sizeof(struct {unsigned x:3;}):9,0?9:sizeof(struct {_Bool x:1;})};return sizeof(a)!=8||a[0]!=4||a[1]!=1;}'
            'width-expression' = 'enum{N=3};struct S{unsigned a:(N?N+2:1);int b:sizeof(int)*2;};int main(void){struct S s={31,-128};return s.a!=31||s.b!=-128;}'
            'typedef-qualified' = 'typedef unsigned int U;struct S{volatile U a:3;const int b:4;};int main(void){struct S s={7,-3};s.a=2;return s.a!=2||s.b!=-3;}'
            'volatile-record' = 'struct S{unsigned a:3,b:5;};int main(void){volatile struct S s={7,17};s.a+=2;return s.a!=1||s.b!=17;}'
            'const-volatile-read' = 'struct S{unsigned a:3;};const volatile struct S s={7};int main(void){return s.a!=7;}'
            'promotions' = 'struct S{unsigned a:31,b:32;_Bool c:1;};int main(void){struct S s={1,1,1};return !(s.a-2<0)||(s.b-2<0)||!(-s.a<0)||(-s.b<0)||sizeof(+s.c)!=4||sizeof(+s.a)!=4;}'
            'wide-conversion' = 'struct S{unsigned a:31,b:32;int c:7;};int main(void){struct S s={2147483647U,4294967295U,-33};long long a=s.a,b=s.b,c=s.c;return a!=2147483647LL||b!=4294967295LL||c!=-33;}'
            'address-side-effects' = 'struct S{unsigned a:5,b:5;};struct S s[2];int n;struct S *next(void){return &s[n++];}int main(void){next()->a=17;next()->b=9;return n!=2||s[0].a!=17||s[0].b||s[1].a||s[1].b!=9;}'
            'adjacent-rhs' = 'struct S{unsigned a:5,b:5;};int main(void){struct S s={7,9};s.a+=(s.b=3);return s.a!=10||s.b!=3;}'
            'volatile-adjacent-rhs' = 'struct S{unsigned a:5,b:5;};int main(void){volatile struct S s={7,9};s.a+=(s.b=3);return s.a!=10||s.b!=3;}'
            'postfix-rhs' = 'struct S{unsigned a:5,b:5;};int main(void){struct S s={7,9};s.a=s.b++;return s.a!=9||s.b!=10;}'
            'assignment-result' = 'struct S{unsigned a:3;int b:3;};int main(void){struct S s={0,0};if((s.a=9)!=1||s.a!=1)return 1;return (s.b=7)!=-1||s.b!=-1;}'
            'increment-wrap' = 'struct S{unsigned a:3;int b:3;};int main(void){struct S s={7,3};if(s.a++!=7||s.a!=0)return 1;if(++s.a!=1)return 2;if(s.b++!=3||s.b!=-4)return 3;return ++s.b!=-3;}'
            'all-compounds' = 'struct S{unsigned a:7,b:7;};int main(void){struct S s={31,77};s.a+=3;s.a-=2;s.a*=3;s.a/=4;s.a%=7;s.a<<=2;s.a>>=1;s.a|=16;s.a&=23;s.a^=3;return s.a!=21||s.b!=77;}'
            'sizeof-values' = 'struct S{unsigned a:3;};int main(void){struct S s={7};return sizeof(s.a+0)!=4||sizeof((unsigned)s.a)!=4||sizeof((0,s.a))!=4||sizeof(s.a=1)!=4||s.a!=7;}'
            'copy' = 'struct S{unsigned a:3;int b:5;};int main(void){struct S a={7,-9},b;b=a;return b.a!=7||b.b!=-9;}'
            'small-return' = 'struct S{unsigned a:3;int b:5;};struct S f(void){struct S s={7,-9};return s;}int main(void){return f().a!=7||f().b!=-9;}'
            'large-return' = 'struct S{unsigned a:31,b:32,c:32;};struct S f(void){struct S s={7,9,11};return s;}int main(void){struct S s=f();return s.a!=7||s.b!=9||s.c!=11;}'
            'argument' = 'struct S{unsigned a:3;int b:5;};int f(struct S s){return s.a+s.b;}int main(void){struct S s={7,-9};return f(s)!=-2;}'
            'flexible-after-field' = 'struct S{unsigned a:3;char data[];};int main(void){return sizeof(struct S)!=4||__offsetof(struct S,data)!=4;}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "bf-$mode-$name" $cases[$name]
        }
        $rejects = @{
            'named-zero' = 'struct S{int a:0;};int main(void){return 0;}'
            'negative-width' = 'struct S{int a:-1;};int main(void){return 0;}'
            'large-width' = 'struct S{unsigned a:33;};int main(void){return 0;}'
            'bool-width' = 'struct S{_Bool a:2;};int main(void){return 0;}'
            'nonconstant-width' = 'int n;struct S{unsigned a:n;};int main(void){return 0;}'
            'pointer' = 'struct S{int *a:3;};int main(void){return 0;}'
            'array' = 'struct S{int a[2]:3;};int main(void){return 0;}'
            'function' = 'struct S{int a(void):3;};int main(void){return 0;}'
            'record' = 'struct I{int x;};struct S{struct I a:3;};int main(void){return 0;}'
            'long-extension' = 'struct S{long a:3;};int main(void){return 0;}'
            'duplicate' = 'struct S{unsigned a:3;int a:3;};int main(void){return 0;}'
            'only-unnamed' = 'struct S{unsigned :3;};int main(void){return 0;}'
            'flexible-after-unnamed' = 'struct S{unsigned :3;char a[];};int main(void){return 0;}'
            'address' = 'struct S{int a:3;};int main(void){struct S s;int *p=&s.a;return 0;}'
            'static-address' = 'struct S{int a:3;}s;int *p=&s.a;int main(void){return 0;}'
            'sizeof' = 'struct S{int a:3;};int main(void){struct S s;return sizeof(s.a);}'
            'sizeof-parens' = 'struct S{int a:3;};int main(void){struct S s;return sizeof((s.a));}'
            'sizeof-return' = 'struct S{int a:3;};struct S f(void){struct S s={0};return s;}int main(void){return sizeof(f().a);}'
            'offsetof' = 'struct S{int a:3;};int main(void){return __offsetof(struct S,a);}'
            'offsetof-nested' = 'struct I{int a:3;};struct S{struct I i;};int main(void){return __offsetof(struct S,i.a);}'
            'const-store' = 'struct S{const unsigned a:3;};int main(void){struct S s={0};s.a=1;return 0;}'
            'const-parent' = 'struct S{unsigned a:3;};int main(void){const struct S s={0};s.a++;return 0;}'
            'const-copy' = 'struct S{const unsigned a:3;};int main(void){struct S a={0},b={1};a=b;return 0;}'
            'extra-initializer' = 'struct S{unsigned :3;unsigned a:4;};int main(void){struct S s={1,2};return 0;}'
            'rvalue-store' = 'struct S{unsigned a:3;};struct S f(void){struct S s={0};return s;}int main(void){f().a=1;return 0;}'
        }
        foreach ($name in ($rejects.Keys | Sort-Object)) {
            Check "bf-$mode-reject-$name" $rejects[$name] 0 $true
        }
        foreach ($shift in 0..7) {
            $pad = ('_Bool :1;' * $shift)
            $src = "struct S{$pad _Bool v:1;};int main(void){volatile struct S s;volatile unsigned char *p=(volatile unsigned char*)&s;int vals[]={-7,-1,0,1,2,17};int i;unsigned int raw=165;if(sizeof(s)!=1)return 1;*p=(unsigned char)raw;for(i=0;i<6;i++){s.v=vals[i];raw=(raw&~(1U<<$shift))|((vals[i]!=0)<<$shift);if(s.v!=(vals[i]!=0)||*p!=raw)return 2;}return 0;}"
            Check "bf-$mode-bool-offset-$shift" $src
        }
        $counter = @'
int loads(unsigned char *p,unsigned char *end,int reg){
    int n=0;while(p+2<end){if((p[0]==139&&p[1]==reg)||(p[0]==15&&p[1]==182&&p[2]==reg))n++;p++;}return n;
}
int writes(unsigned char *p,unsigned char *end){
    int n=0;while(p+1<end){if((p[0]==137||p[0]==136)&&(p[1]==1||p[1]==17))n++;p++;}return n;
}
'@
        $accesses = @(
            @('store', 's.a=1;', 0, 1),
            @('read', 's.a;', 1, 0),
            @('void-read', '(void)s.a;', 1, 0),
            @('prefix', '++s.a;', 1, 1),
            @('postfix', 's.a++;', 1, 1),
            @('compound', 's.a+=1;', 1, 1),
            @('assignment-result', '(s.a=1)+0;', 0, 1),
            @('sizeof-promoted', 'sizeof(+s.a);', 0, 0)
        )
        foreach ($field in @(@('signed3', 'int', 3), @('signed31', 'int', 31), @('unsigned32', 'unsigned', 32), @('bool1', '_Bool', 1))) {
            foreach ($item in $accesses) {
                $preserve = if ($field[2] -eq 32) {
                    0
                }
                else {
                    $item[3]
                }
                $src = 'struct S{' + $field[1] + ' a:' + $field[2] + ';};volatile struct S s;int f(void){' + $item[1] + 'return 0;}int end(void){return 0;}' + $counter +
                'int main(void){unsigned char *p=(unsigned char*)f,*e=(unsigned char*)end;f();return loads(p,e,0)!=' + $item[2] + '||loads(p,e,17)!=' + $preserve + '||writes(p,e)!=' + $item[3] + ';}'
                Check ("bf-$mode-access-" + $field[0] + '-' + $item[0]) $src
            }
        }
        $src = 'struct S{unsigned a:3,b:5;};volatile struct S s;int f(void){s.a+=(s.b=1);return 0;}int end(void){return 0;}' + $counter +
        'int main(void){unsigned char *p=(unsigned char*)f,*e=(unsigned char*)end;f();return s.a!=1||s.b!=1||loads(p,e,0)!=1||loads(p,e,17)!=2||writes(p,e)!=2;}'
        Check "bf-$mode-access-adjacent-rhs" $src
        foreach ($kind in 'unsigned', 'signed') {
            foreach ($width in 1..32) {
                $functions = @()
                $calls = @()
                foreach ($shift in 0..(32 - $width)) {
                    $pad = if ($shift) {
                        "unsigned :$shift;"
                    }
                    else {
                        ''
                    }
                    $tail = 32 - $width - $shift
                    if ($tail) {
                        $suffix = "unsigned :$tail;"
                    }
                    else {
                        $suffix = ''
                    }
                    $base = if ($kind -eq 'unsigned') {
                        'unsigned'
                    }
                    else {
                        'int'
                    }
                    $values = if ($kind -eq 'unsigned') {
                        'unsigned long long values[]={0,1,31,2147483647ULL,2147483648ULL,4294967295ULL,4294967296ULL};'
                    }
                    else {
                        'long long values[]={-2147483647LL-1,-31,-1,0,1,31,2147483647LL};'
                    }
                    $expect = if ($kind -eq 'unsigned') {
                        'long long expected=(long long)bits;'
                    }
                    else {
                        "long long expected=(bits&(1ULL<<($width-1)))?(long long)bits-(long long)(1ULL<<$width):(long long)bits;"
                    }
                    $functions += @"
int f$shift(void){
    struct S{$pad $base v:$width;$suffix};volatile struct S s;
    volatile unsigned char *p=(volatile unsigned char*)&s;
    unsigned long long mask=(1ULL<<$width)-1,raw=0xa5a5a5a5ULL,bits,got;
    int i,j;$values
    if(sizeof(s)!=4)return 1;
    for(j=0;j<4;j++)p[j]=(unsigned char)(raw>>(j*8));
    bits=(raw>>$shift)&mask;
    {$expect if((long long)s.v!=expected)return 2;}
    for(i=0;i<7;i++){
        s.v=values[i];bits=(unsigned long long)values[i]&mask;
        {$expect if((long long)s.v!=expected)return 3;}
        raw=(raw&~(mask<<$shift))|(bits<<$shift);
        got=0;for(j=0;j<4;j++)got|=(unsigned long long)p[j]<<(j*8);
        if(got!=raw)return 4;
    }
    return 0;
}
"@
                    $calls += "if(f$shift())return " + ($shift + 1) + ';'
                }
                Check "bf-$mode-$kind-width-$width-all-offsets" (($functions -join [char]10) + 'int main(void){' + ($calls -join '') + 'return 0;}')
            }
        }
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
