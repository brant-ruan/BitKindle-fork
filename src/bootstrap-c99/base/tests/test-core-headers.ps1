$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    PpCheck 'headers-stddef-types' @('#include <stddef.h>', 'int main(void){return sizeof(size_t)+sizeof(ptrdiff_t)+sizeof(wchar_t);}') 18
    PpCheck 'headers-size-t-type' @('#include <stddef.h>', 'int main(void){size_t n=(size_t)-1;return n>0 && sizeof n==sizeof(sizeof(int));}') 1
    PpCheck 'headers-ptrdiff-type' @('#include <stddef.h>', 'int main(void){int a[2];ptrdiff_t n=&a[0]-&a[1];return n==-1 && sizeof n==8;}') 1
    PpCheck 'headers-null-object' @('#include <stddef.h>', 'int main(void){int *p=NULL;return p==0;}') 1
    PpCheck 'headers-null-function' @('#include <stddef.h>', 'int main(void){int (*p)(void)=NULL;return p==0;}') 1
    PpCheck 'headers-wchar-range' @('#include <stddef.h>', 'int main(void){wchar_t c=65535;return c==65535 && c>0;}') 1
    PpCheck 'offsetof-first' @('#include <stddef.h>', 'struct S{char a;int b;};int main(void){return offsetof(struct S,a);}') 0
    PpCheck 'offsetof-alignment' @('#include <stddef.h>', 'struct S{char a;long long b;};int main(void){return offsetof(struct S,b);}') 8
    PpCheck 'offsetof-size-t' @('#include <stddef.h>', 'struct S{int a;};int main(void){return sizeof offsetof(struct S,a);}') 8
    PpCheck 'offsetof-enum' @('#include <stddef.h>', 'struct S{char a;int b;};enum E{N=offsetof(struct S,b)};int main(void){return N;}') 4
    PpCheck 'offsetof-array-bound' @('#include <stddef.h>', 'struct S{char a;int b;};int a[offsetof(struct S,b)];int main(void){return sizeof a;}') 16
    PpCheck 'offsetof-case' @('#include <stddef.h>', 'struct S{char a;int b;};int main(void){switch(4){case offsetof(struct S,b):return 7;}return 0;}') 7
    PpCheck 'offsetof-static' @('#include <stddef.h>', 'struct S{char a;int b;};size_t n=offsetof(struct S,b);int main(void){return n;}') 4
    PpCheck 'offsetof-nested' @('#include <stddef.h>', 'struct T{char a;int b;};struct S{long long x;struct T t;};int main(void){return offsetof(struct S,t.b);}') 12
    PpCheck 'offsetof-array-element' @('#include <stddef.h>', 'struct S{char c;int a[4];};int main(void){return offsetof(struct S,a[2]);}') 12
    PpCheck 'offsetof-matrix' @('#include <stddef.h>', 'struct S{char c;int a[2][3];};int main(void){return offsetof(struct S,a[1][2]);}') 24
    PpCheck 'offsetof-array-record' @('#include <stddef.h>', 'struct T{char a;int b;};struct S{long long x;struct T a[3];};int main(void){return offsetof(struct S,a[2].b);}') 28
    PpCheck 'offsetof-index-expression' @('#include <stddef.h>', 'struct S{int a[4];};int main(void){return offsetof(struct S,a[1+2]);}') 12
    PpCheck 'offsetof-typedef' @('#include <stddef.h>', 'typedef struct {char a;int b;} S;int main(void){return offsetof(S,b);}') 4
    PpCheck 'offsetof-flexible' @('#include <stddef.h>', 'struct S{long long n;char c;char data[];};int main(void){return offsetof(struct S,data);}') 9
    PpCheck 'offsetof-union' @('#include <stddef.h>', 'union U{char a;long long b;};int main(void){return offsetof(union U,b);}') 0
    PpCheck 'offsetof-nested-constant' @('#include <stddef.h>', 'struct T{int a,b;};struct S{int a[8];};int main(void){return offsetof(struct S,a[offsetof(struct T,b)]);}') 16
    PpCheck 'offsetof-reject-pointer-type' @('#include <stddef.h>', 'struct S{int n;};int main(void){return offsetof(struct S*,n);}') 0 $true
    PpCheck 'offsetof-reject-missing-member' @('#include <stddef.h>', 'struct S{int n;};int main(void){return offsetof(struct S,x);}') 0 $true
    PpCheck 'offsetof-reject-incomplete' @('#include <stddef.h>', 'struct S;int main(void){return offsetof(struct S,x);}') 0 $true
    PpCheck 'offsetof-reject-pointer-subscript' @('#include <stddef.h>', 'struct S{int *a;};int main(void){return offsetof(struct S,a[1]);}') 0 $true
    PpCheck 'offsetof-reject-variable-index' @('#include <stddef.h>', 'struct S{int a[3];};int main(void){int n=1;return offsetof(struct S,a[n]);}') 0 $true
    PpCheck 'offsetof-reject-negative-index' @('#include <stddef.h>', 'struct S{int a[3];};int main(void){return offsetof(struct S,a[-1]);}') 0 $true
    PpCheck 'offsetof-reject-overflow' @('#include <stddef.h>', 'struct S{long long a[3];};int main(void){return offsetof(struct S,a[18446744073709551615ULL]);}') 0 $true
    PpCheck 'headers-limits-char' @('#include <limits.h>', 'int main(void){return CHAR_BIT==8 && CHAR_MIN==-128 && CHAR_MAX==127 && SCHAR_MIN==-128 && SCHAR_MAX==127 && UCHAR_MAX==255;}') 1
    PpCheck 'headers-limits-short' @('#include <limits.h>', 'int main(void){return SHRT_MIN==-32768 && SHRT_MAX==32767 && USHRT_MAX==65535;}') 1
    PpCheck 'headers-limits-int' @('#include <limits.h>', 'int main(void){return INT_MIN==(-2147483647-1) && INT_MAX==2147483647 && UINT_MAX==0xffffffffU;}') 1
    PpCheck 'headers-limits-long' @('#include <limits.h>', 'int main(void){return LONG_MIN==(-2147483647L-1L) && LONG_MAX==2147483647L && ULONG_MAX==0xffffffffUL;}') 1
    PpCheck 'headers-limits-wide' @('#include <limits.h>', 'int main(void){return LLONG_MIN==(-9223372036854775807LL-1LL) && LLONG_MAX==9223372036854775807LL && ULLONG_MAX==~0ULL;}') 1
    PpCheck 'headers-limits-preprocessor' @('#include <limits.h>', '#if INT_MIN >= 0 || UINT_MAX != 4294967295U || CHAR_BIT != 8 || MB_LEN_MAX != 1', '#error incorrect limits', '#endif', 'int main(void){return 0;}')
    PpCheck 'headers-limits-expression-types' @('#include <limits.h>', 'int main(void){return sizeof(INT_MIN)==4 && sizeof(LONG_MIN)==4 && sizeof(LLONG_MIN)==8 && -UINT_MAX>0 && -ULLONG_MAX>0;}') 1
    PpCheck 'headers-bool' @('#include <stdbool.h>', 'int main(void){bool b=7;return b==true && !false && sizeof b==1 && __bool_true_false_are_defined;}') 1
    PpCheck 'headers-bool-preprocessor' @('#include <stdbool.h>', '#if !true || false || !__bool_true_false_are_defined', '#error bool macros', '#endif', 'int main(void){return 0;}')
    PpCheck 'headers-iso646-logical' @('#include <iso646.h>', 'int main(void){return (1 and not 0) or (1 not_eq 1);}') 1
    PpCheck 'headers-iso646-bits' @('#include <iso646.h>', 'int main(void){int n=7;n and_eq 3;n or_eq 8;n xor_eq 1;return (n bitand 15) bitor ((compl 0 xor -1));}') 10
    PpCheck 'headers-reinclude' @('#include <stddef.h>', '#include <limits.h>', '#include <stdbool.h>', '#include <iso646.h>', '#include <stddef.h>', '#include <limits.h>', '#include <stdbool.h>', '#include <iso646.h>', 'int main(void){size_t n=sizeof(bool);return n==1 and CHAR_BIT==8;}') 1
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
}
