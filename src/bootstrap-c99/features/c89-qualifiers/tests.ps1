$saved = $env:C1_STANDARD
$duplicates = [ordered]@{
    'const' = 'const const int x=42;int main(void){return x!=42;}'
    'volatile' = 'volatile volatile int x=42;int main(void){return x!=42;}'
    'separated' = 'const int const x=42;int main(void){return x!=42;}'
    'pointer' = 'int x=42;int *const const p=&x;int main(void){return *p!=42;}'
    'typedef' = 'typedef const int T;const T x=42;int main(void){return x!=42;}'
    'typedef-array' = 'typedef const int T[2];const T x={20,22};int main(void){return x[0]+x[1]!=42;}'
    'typedef-pointer' = 'typedef int *const T;int x=42;const T p=&x;int main(void){return *p!=42;}'
    'member' = 'struct S{const const int x;};int main(void){struct S s={42};return s.x!=42;}'
    'parameter' = 'int f(const const int x){return x;}int main(void){return f(42)!=42;}'
    'abstract' = 'int main(void){return sizeof(const const int)!=4;}'
}
try {
    foreach ($mode in 'c89', 'c99') {
        $env:C1_STANDARD = $mode
        foreach ($name in $duplicates.Keys) {
            Check "qualifier-$mode-$name" $duplicates[$name] 0 ($mode -eq 'c89')
        }
        Check "qualifier-$mode-separate-levels" 'const int x=42;const int *const p=&x;int main(void){return *p!=42;}' 0
        Check "qualifier-$mode-distinct" 'const volatile int x=42;int main(void){return x!=42;}' 0
        Check "qualifier-$mode-typedef-pointee" 'typedef const int *P;const int x=42;const P p=&x;int main(void){return *p!=42;}' 0
        Check "qualifier-$mode-redeclaration" 'extern const int x;extern const int x;const int x=42;int main(void){return x!=42;}' 0
        Check "qualifier-$mode-record-separate-declarations" 'const struct S{const int x;} s={42};int main(void){return s.x!=42;}' 0
        Check "qualifier-$mode-array-distinct" 'typedef const int T[2];volatile T x={20,22};int main(void){return x[0]+x[1]!=42;}' 0
    }
}
finally {
    $env:C1_STANDARD = $saved
}
