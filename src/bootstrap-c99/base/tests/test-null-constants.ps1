$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $zeros = @(
            '1-1', '+0', '-0', '!1', '~(-1)', '2*3-6', '8/3-2', '8%3-2',
            '7&8', '7^7', '0|0', '1==2', '1!=1', '2<1', '1>2', '2<=1', '1>=2',
            '(1<<5)-32', '(32>>5)-1', '0U-0U', '0xffffffffU+1U',
            '0xffffffffffffffffULL+1ULL', '0x100000000ULL-0x100000000ULL',
            '(unsigned char)256', '(signed char)256', '(unsigned short)65536',
            '(int)0x100000000ULL', '(unsigned int)0x100000000ULL',
            '(long)0x100000000ULL', '(_Bool)0', '(enum E)0', 'Z',
            'sizeof(int)-4', 'sizeof(long long)-8', 'sizeof(struct S)-8',
            'sizeof(f())-sizeof(int)', '__offsetof(struct S,a)',
            '0&&1', '(1||0)-1', '0&&(1/0)', '(1||(1/0))-1',
            '1?0:1', '0?1:0', '1?0:(1/0)', '0?(1/0):0',
            '(0?0ULL:0U)', '0&&(1<<32)', '1?0:(2147483647+1)',
            '(1?(unsigned char)256:1)', '(0?1:(unsigned short)65536)',
            'sizeof((f(),f()))-4', 'sizeof(*(int*)0)-4',
            'sizeof(((struct B*)0)->a+0)-4'
        )
        $prefix = 'enum E{Z=0};struct S{int a,b;};struct B{unsigned a:3;};int calls;int f(void){calls++;return 3;}'
        $index = 0
        foreach ($z in $zeros) {
            $index++
            $src = $prefix + 'int *r(void){return (' + $z + ');}int take(int*p){return p!=0;}int main(void){int x=7;int *p=(' + $z + ');if(p||r()||take((' + $z + ')))return 1;p=&x;p=(' + $z + ');if(p)return 2;if(p!=(' + $z + ')||(' + $z + ')!=p)return 3;p=1?&x:(' + $z + ');if(*p!=7)return 4;p=0?(' + $z + '):&x;return *p!=7||calls;}'
            Check "npc-$mode-zero-$index" $src
            $src = $prefix + 'int (*r(void))(void){return (' + $z + ');}int take(int(*p)(void)){return p!=0;}int main(void){int(*p)(void)=(' + $z + ');if(p||r()||take((' + $z + ')))return 1;p=f;p=(' + $z + ');if(p)return 2;if(p!=(' + $z + ')||(' + $z + ')!=p)return 3;p=1?f:(' + $z + ');if(p()!=3)return 4;p=0?(' + $z + '):f;return p()!=3||calls!=2;}'
            Check "npc-$mode-function-zero-$index" $src
            Check "npc-$mode-static-zero-$index" ($prefix + 'int*p=(' + $z + ');int(*q)(void)=(' + $z + ');int main(void){return p!=0||q!=0||calls;}')
        }
        $valid = @{
            'void-zero' = 'int f(void){return 4;}int main(void){int(*p)(void)=(void*)(3-3);return p!=0||((void*)(3-3))!=p;}'
            'void-conditional-object' = 'int main(void){int x=7;int*p=&x;return *(1?p:(void*)(1-1))!=7||*(0?(void*)(2-2):p)!=7;}'
            'void-conditional-function' = 'int f(void){return 9;}int main(void){return (1?f:(void*)(1-1))()!=9||(0?(void*)(2-2):f)()!=9;}'
            'conditional-qualifiers' = 'int main(void){const int x=7;const int*p=&x;return *(1?p:(void*)(1-1))!=7;}'
            'truncated-nonzero' = 'int main(void){unsigned char x=(unsigned char)257;return x!=1;}'
            'runtime-comma' = 'int main(void){int x=2;int y=(x++,0);return x!=3||y;}'
            'runtime-zero' = 'int main(void){int x=2;return x-x;}'
            'dead-invalid-runtime' = 'int main(void){return 1?0:(1/0);}'
            'integer-cast-zero' = 'int main(void){int*p=(unsigned char)256;return p!=0;}'
            'sizeof-side-effects' = 'int main(void){int x=3;int*p=sizeof(x++)-4;return x!=3||p!=0;}'
            'inferred-array-state' = 'int main(void){int *a[]={1-1,sizeof(struct X{int n;})-4};return sizeof(a)!=16||a[0]||a[1];}'
            'global-zero' = 'int*p=1-1;int(*f)(void)=(void*)(2-2);int main(void){return p!=0||f!=0;}'
            'null-stays-pointer' = 'int main(void){int *p=0;int *q=(int*)(2-2);return p!=q;}'
            'unselected-comma' = 'int main(void){int *p=1?0:(1,2);int *q=0&&(1,2);return p!=0||q!=0;}'
            'unevaluated-overflow' = 'int f(void){return 2147483647+1;}int main(void){return 0;}'
            'unevaluated-division' = 'int f(void){return 1/0;}int main(void){return 0;}'
            'constant-context-isolation' = 'enum{A=3+sizeof(2*3),B=sizeof(1?2:3)+4};int a[A];int main(void){int*p=sizeof(a)-28;return p!=0||A!=7||B!=8;}'
            'volatile-no-fold' = 'int main(void){volatile int z=0;return z-z;}'
            'aggregate-pointer-fields' = 'struct S{int*p;int(*f)(void);};int main(void){struct S s={7-7,(void*)(9-9)};return s.p!=0||s.f!=0;}'
            'nested-conditional' = 'int main(void){int*p=1?(0?1:0):(1?2:3);return p!=0;}'
            'unsigned-long-promotions' = 'int main(void){int*p=(0xffffffffUL+1UL);int*q=((-1LL+1ULL));return p!=0||q!=0;}'
            'static-unselected-comma' = 'int*p=1?0:(1,2);int(*q)(void)=0&&(1,2);int main(void){return p!=0||q!=0;}'
            'unparenthesized-middle-comma' = 'int*p=0?1,2:0;int main(void){int*q=0?1,2:0;return p!=0||q!=0;}'
            'enum-unselected-comma' = 'enum{A=1?3:(4,5),B=0?7,8:4};int a[A];int main(void){return A!=3||B!=4||sizeof(a)!=12;}'
            'static-conditional-pointer-arithmetic' = 'int a[3];int*p=(1?a:(void*)(1-1))+2;int*q=(0?(void*)(1-1):a)+1;int main(void){return p!=a+2||q!=a+1;}'
            'static-conditional-qualified-pointer' = 'const int a[2]={3,7};const int*p=(1?a:(void*)0)+1;int main(void){return *p!=7;}'
        }
        foreach ($name in $valid.Keys) {
            Check "npc-$mode-$name" $valid[$name]
        }
        $bad = @(
            '1+1', '!0', '~0', '(unsigned char)257', '(_Bool)256',
            '(int)1ULL', '2147483647+1', '(-2147483647-1)/-1',
            '(-2147483647-1)%-1', '1/0', '0/0', '0%0', '1<<32',
            '0<<32', '0>>32', '0<<-1', '(-1)<<1', '1<<31',
            '(-2147483647-1)-1', '2147483647*2', '-(-2147483647-1)',
            '0x7fffffffffffffffLL+1LL', '(-9223372036854775807LL-1LL)/-1LL',
            '(-9223372036854775807LL-1LL)%-1LL', '1LL<<63', '0ULL<<64',
            '(1,0)', '0?0:(1,0)', '(void*)1'
        )
        $index = 0
        foreach ($z in $bad) {
            $index++
            # A nonzero void pointer can convert to an object pointer, but never to a function pointer.
            Check "npc-$mode-reject-$index" ('int main(void){int(*p)(void)=(' + $z + ');return p!=0;}') 0 $true
        }
        $reject = @{
            'variable' = 'int main(void){int z=0;int*p=z;return 0;}'
            'const-variable' = 'int main(void){const int z=0;int*p=z;return 0;}'
            'variable-difference' = 'int main(void){int z=1;int*p=z-z;return 0;}'
            'assignment' = 'int main(void){int z;int*p=(z=0);return 0;}'
            'function' = 'int f(void){return 0;}int main(void){int*p=f();return 0;}'
            'member' = 'struct S{int a;};int main(void){struct S s={0};int*p=s.a;return 0;}'
            'bitfield' = 'struct S{unsigned a:3;};int main(void){struct S s={0};int*p=s.a;return 0;}'
            'cast-variable' = 'int main(void){int x=256;int*p=(unsigned char)x;return 0;}'
            'variable-condition' = 'int main(void){int x=1;int*p=x?0:0;return 0;}'
            'pointer-integer-roundtrip' = 'int main(void){int*p=(long long)(void*)0;return 0;}'
            'pointer-unary-not' = 'int main(void){int*p=!(void*)1;return 0;}'
            'void-variable-function' = 'int main(void){int x=0;int(*p)(void)=(void*)x;return 0;}'
            'void-constant-nonzero-function' = 'int main(void){int(*p)(void)=(void*)(2-1);return 0;}'
            'qualified-void-function' = 'int main(void){int(*p)(void)=(const void*)(1-1);return 0;}'
            'conditional-pointer-not-npc' = 'int main(void){int(*p)(void)=1?(void*)0:(void*)0;return 0;}'
            'comma-pointer-not-npc' = 'int main(void){int(*p)(void)=(1,(void*)0);return 0;}'
            'integer-conditional-nonzero' = 'int main(void){int*p=0?0:1;return 0;}'
            'runtime-logical-zero' = 'int main(void){int x=1;int*p=x&&0;return 0;}'
            'overflow-truncate' = 'int main(void){int*p=(unsigned char)(2147483647+1);return 0;}'
            'invalid-short-selected' = 'int main(void){int*p=1&&(1/0);return 0;}'
            'invalid-condition' = 'int main(void){int*p=(1/0)?0:0;return 0;}'
            'pointer-comparison-integer' = 'int main(void){int*p=(void*)0!=(void*)0;return 0;}'
            'volatile-zero' = 'int main(void){volatile int z=0;int*p=z;return 0;}'
            'pointer-comma-under-cast' = 'int main(void){int*p=(int)((void*)0,0);return 0;}'
            'static-selected-comma' = 'int*p=(1,0);int main(void){return 0;}'
            'enum-selected-comma' = 'enum{A=(1,0)};int main(void){return 0;}'
            'short-unselected-variable' = 'int main(void){int x=0;int*p=0&&x;return 0;}'
            'conditional-unselected-variable' = 'int main(void){int x=0;int*p=1?0:x;return 0;}'
            'short-unselected-call' = 'int f(void){return 0;}int main(void){int*p=0&&f();return 0;}'
            'short-unselected-pointer' = 'int main(void){int*p=0&&(void*)0;return 0;}'
        }
        foreach ($name in $reject.Keys) {
            Check "npc-$mode-reject-$name" $reject[$name] 0 $true
        }
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
