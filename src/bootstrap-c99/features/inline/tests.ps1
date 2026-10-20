Check 'inline-static-basic' 'static inline int f(int x){return x+1;}int main(void){return f(41)!=42;}' 0
Check 'inline-static-repeated' 'inline static inline int f(void){return 42;}int main(void){return f()!=42;}' 0
Check 'inline-static-address' 'static inline int f(void){return 42;}int main(void){int (*p)(void)=f;return p()!=42||p!=f;}' 0
Check 'inline-static-local-state' 'static inline int f(void){static int n;return ++n;}int main(void){return f()!=1||f()!=2;}' 0
Check 'inline-static-private-reference' 'static int x=42;static inline int f(void){return x;}int main(void){return f()!=42;}' 0
Check 'inline-extern-definition' 'extern inline int f(void){return 42;}int main(void){return f()!=42;}' 0
Check 'inline-extern-later' 'inline int f(void){return 42;}extern int f(void);int main(void){return f()!=42;}' 0
Check 'inline-ordinary-earlier' 'int f(void);inline int f(void){return 42;}int main(void){return f()!=42;}' 0
Check 'inline-ordinary-later' 'inline int f(void){return 42;}int f(void);int main(void){return f()!=42;}' 0
Check 'inline-prototype-before-definition' 'inline int f(void);int f(void){return 42;}int main(void){return f()!=42;}' 0
Check 'inline-function-typedef' 'typedef int F(void);static inline F f;static int f(void){return 42;}int main(void){return f()!=42;}' 0
Check 'inline-multiple-declarators' 'static inline int f(void),g(void);static int f(void){return 20;}static int g(void){return 22;}int main(void){return f()+g()!=42;}' 0
Check 'inline-object-rejected' 'inline int object;int main(void){return 0;}' 0 $true
Check 'inline-pointer-object-rejected' 'inline int (*p)(void);int main(void){return 0;}' 0 $true
Check 'inline-typedef-rejected' 'typedef inline int F(void);int main(void){return 0;}' 0 $true
Check 'inline-parameter-rejected' 'int f(inline int n){return n;}int main(void){return 0;}' 0 $true
Check 'inline-main-rejected' 'inline int main(void){return 0;}' 0 $true
Check 'inline-external-modifiable-static-rejected' 'inline int f(void){static int n;return ++n;}int main(void){return 0;}' 0 $true
Check 'inline-external-internal-reference-rejected' 'static int x;inline int f(void){return x;}int main(void){return 0;}' 0 $true
Check 'inline-const-static-allowed' 'inline int f(void){static const int n=42;return n;}int main(void){return 0;}' 0
Check 'inline-const-array-allowed' 'inline int f(void){static const int n[2]={20,22};return n[0]+n[1];}int main(void){return 0;}' 0
Check 'inline-shadow-private-object' 'static int n;inline int f(int n){return n;}int main(void){return 0;}' 0
Check 'inline-shadow-private-function' 'static int f(void){return 1;}inline int g(void){int f=42;return f;}int main(void){return 0;}' 0
Check 'inline-later-extern-static-state' 'inline int f(void){static int n;return ++n;}extern int f(void);int main(void){return f()!=1||f()!=2;}' 0
Check 'inline-later-extern-private-reference' 'static int n=42;inline int f(void){return n;}extern int f(void);int main(void){return f()!=42;}' 0
Check 'inline-private-function-rejected' 'static int f(void){return 42;}inline int g(void){return f();}int main(void){return 0;}' 0 $true
Check 'inline-private-address-rejected' 'static int n;inline int *f(void){return &n;}int main(void){return 0;}' 0 $true
Check 'inline-static-pointer-nonconst-rejected' 'inline int f(void){static const int *p;return p!=0;}int main(void){return 0;}' 0 $true
Check 'inline-type-name-rejected' 'int main(void){return sizeof(inline int);}' 0 $true
Check 'inline-cast-rejected' 'int main(void){return (inline int)0;}' 0 $true
Check 'inline-member-rejected' 'struct S{inline int n;};int main(void){return 0;}' 0 $true
Check 'inline-block-extern-object-rejected' 'int n;int main(void){extern inline int n;return 0;}' 0 $true
Check 'inline-tag-only-rejected' 'inline struct S{int n;};int main(void){return 0;}' 0 $true
Check 'inline-block-main-rejected' 'int main(void){inline int main(void);return 0;}' 0 $true
Check 'inline-block-prototype' 'int f(void){return 42;}int main(void){inline int f(void);return f()!=42;}' 0
Check 'inline-block-extern-prototype' 'int f(void){return 42;}int main(void){extern inline int f(void);return f()!=42;}' 0
Check 'inline-struct-return-specifier-restored' 'inline struct S{int n;} f(void){struct S s={42};return s;}extern struct S f(void);int main(void){return f().n!=42;}' 0
