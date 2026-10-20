$savedStandard = $env:C1_STANDARD
try {
    $env:C1_STANDARD = 'c89'
    Check 'c89-inline-local' 'int main(void){int inline=42;return inline!=42;}' 0
    Check 'c89-restrict-local' 'int main(void){int restrict=42;return restrict!=42;}' 0
    Check 'c89-inline-function' 'int inline(int restrict){return restrict;}int main(void){return inline(42)!=42;}' 0
    Check 'c89-restrict-typedef' 'typedef int restrict;int main(void){restrict inline=42;return inline!=42;}' 0
    Check 'c89-inline-tag-member' 'struct inline{int restrict;};int main(void){struct inline a;a.restrict=42;return a.restrict!=42;}' 0
    Check 'c89-inline-label' 'int main(void){goto inline;return 1;inline:return 0;}' 0
    Check 'c89-inline-macro' "#define inline 42`nint main(void){return inline!=42;}" 0
    Check 'c89-division-before-comment' 'int main(void){return 84//**/2!=42;}' 0
    Check 'c89-macro-division-before-comment' "#define HALF(x) x//**/2`nint main(void){return HALF(84)!=42;}" 0
    Check 'c89-stringize-slashes' "#define S(x) #x`nint main(void){char *p=S(//);return sizeof(S(//))!=3||p[0]!=47||p[1]!=47;}" 0
    $env:C1_STANDARD = 'c99'
    Check 'c99-inline-still-keyword' 'static inline int f(void){return 42;}int main(void){return f()!=42;}' 0
    Check 'c99-restrict-still-keyword' 'int main(void){int n=42;int *restrict p=&n;return *p!=42;}' 0
    Check 'c99-line-comment' "int main(void){return 0;// ignored`n}" 0
    Check 'c99-inline-object-rejected' 'int main(void){int inline=42;return inline;}' 0 $true
    Check 'c99-restrict-object-rejected' 'int main(void){int restrict=42;return restrict;}' 0 $true
    $env:C1_STANDARD = $null
    Check 'default-inline-extension' 'static inline int f(void){return 42;}int main(void){return f()!=42;}' 0
}
finally {
    $env:C1_STANDARD = $savedStandard
}
