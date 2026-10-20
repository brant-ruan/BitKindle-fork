Check 'flex-size-char' 'struct S{int n;char data[];};int main(void){return sizeof(struct S);}' 4
Check 'flex-size-aligned' 'struct S{char n;long long data[];};int main(void){return sizeof(struct S);}' 8
Check 'flex-tail-padding' 'struct S{long long n;char c;char data[];};int main(void){struct S s;return sizeof s+((char*)s.data-(char*)&s);}' 25
Check 'flex-padding-access' 'struct S{long long n;char c;char data[];};int main(void){struct S s;s.data[0]=3;s.data[6]=7;return s.data[0]+s.data[6];}' 10
Check 'flex-arrow-access' 'struct S{long long n;char c;int data[];};int main(void){struct S s;struct S*p=&s;p->data[0]=17;return p->data[0];}' 17
Check 'flex-size-element' 'struct S{int n;long long data[];};int main(void){struct S s;return sizeof s.data[0];}' 8
Check 'flex-pointer-member' 'struct S{int n;int *data[];};int main(void){return sizeof(struct S);}' 8
Check 'flex-matrix-member' 'struct S{char n;int data[][3];};int main(void){struct S s;return sizeof s+sizeof s.data[0];}' 16
Check 'flex-typedef-array' 'typedef int A[];struct S{char n;A data;};int main(void){return sizeof(struct S);}' 4
Check 'flex-shared-declaration' 'struct S{int n,data[];};int main(void){return sizeof(struct S);}' 4
Check 'flex-local-initializer' 'struct S{int n;char data[];};int main(void){struct S s={7};return s.n;}' 7
Check 'flex-global-initializer' 'struct S{int n;char data[];};struct S s={9};int main(void){return s.n;}' 9
Check 'flex-trailing-comma' 'struct S{int n;char data[];};struct S s={11,};int main(void){return s.n;}' 11
Check 'flex-static-local' 'struct S{int n;char data[];};int f(void){static struct S s={3};return ++s.n;}int main(void){f();return f();}' 5
Check 'flex-assignment' 'struct S{int n;char data[];};int main(void){struct S a={3},b={7};a=b;return a.n;}' 7
Check 'flex-parameter-return' 'struct S{int n;char data[];};struct S f(struct S s){s.n+=5;return s;}int main(void){struct S s={7};return f(s).n;}' 12
Check 'flex-large-parameter-return' 'struct S{long long a,b;char data[];};struct S f(struct S s){s.b+=5;return s;}int main(void){struct S s={3,7};return f(s).b;}' 12
Check 'flex-union-member' 'struct S{int n;char data[];};union U{struct S s;long long v;};int main(void){union U u;u.s.n=7;return sizeof u+u.s.n;}' 15
Check 'flex-union-initializer' 'struct S{int n;char data[];};union U{struct S s;long long v;};union U u={{7}};int main(void){return u.s.n;}' 7
Check 'flex-recursive-union' 'struct S{int n;char data[];};union U{struct S s;};union V{union U u;long long n;};int main(void){return sizeof(union V);}' 8
Check 'flex-pointer-array' 'struct S{int n;char data[];};struct S *a[2];int main(void){return sizeof a;}' 16
Check 'flex-pointer-nested' 'struct S{int n;char data[];};struct T{struct S *p;};int main(void){return sizeof(struct T);}' 8
Check 'flex-initializer-checkpoint' 'struct W{int n;};int main(void){struct W a[]={{sizeof(struct S{int n;char data[];})},{3}};struct S s={7};return sizeof a+s.n;}' 15
Check 'flex-scalar-inference-definition' 'int main(void){int a[]={sizeof(struct S{int n;char data[];}),3};struct S s={7};return sizeof a+s.n;}' 15
Check 'flex-global-inference-definition' 'int a[]={sizeof(struct S{int n;char data[];}),3};struct S s={7};int main(void){return sizeof a+s.n;}' 15
Check 'shape-nested-record-definition' 'int main(void){int a[]={sizeof(struct S{struct T{int n;} t;int m;}),3};return sizeof a+a[0];}' 16
Check 'shape-reject-mismatched-brace' 'int main(void){int a[]={sizeof(struct S{int n;])};return 0;}' 0 $true
Check 'shape-reject-semicolon-expression' 'int main(void){int a[]={(1;2)};return 0;}' 0 $true
Check 'flex-union-flat-initializer' 'struct S{int n;char data[];};union U{struct S s;};union U u={7};int main(void){return u.s.n;}' 7
Check 'flex-checkpoint-constraint' 'struct W{int n;};int main(void){struct W a[]={{sizeof(struct S{int n;char data[];})}};struct T{struct S s;};return 0;}' 0 $true
Check 'flex-reject-only-member' 'struct S{int data[];};int main(void){return 0;}' 0 $true
Check 'flex-reject-union' 'union U{int n;int data[];};int main(void){return 0;}' 0 $true
Check 'flex-reject-following-member' 'struct S{int n;int data[];int tail;};int main(void){return 0;}' 0 $true
Check 'flex-reject-following-declarator' 'struct S{int n;int data[],tail;};int main(void){return 0;}' 0 $true
Check 'flex-reject-array' 'struct S{int n;char data[];};struct S a[2];int main(void){return 0;}' 0 $true
Check 'flex-reject-union-array' 'struct S{int n;char data[];};union U{struct S s;};union U a[2];int main(void){return 0;}' 0 $true
Check 'flex-reject-nested' 'struct S{int n;char data[];};struct T{struct S s;};int main(void){return 0;}' 0 $true
Check 'flex-reject-union-nested' 'struct S{int n;char data[];};union U{struct S s;};union V{union U u;};struct T{union V v;};int main(void){return 0;}' 0 $true
Check 'flex-reject-sizeof' 'struct S{int n;char data[];};int main(void){struct S s;return sizeof s.data;}' 0 $true
Check 'flex-reject-initializer' 'struct S{int n;char data[];};struct S s={1,{2}};int main(void){return 0;}' 0 $true
Check 'flex-reject-string-initializer' 'struct S{int n;char data[];};struct S s={1,"abc"};int main(void){return 0;}' 0 $true
Check 'flex-reject-local-initializer' 'struct S{int n;char data[];};int main(void){struct S s={1,{2}};return 0;}' 0 $true
Check 'flex-reject-element-incomplete' 'struct T;struct S{int n;struct T data[];};int main(void){return 0;}' 0 $true
Check 'flex-reject-flexible-element' 'struct S{int n;char data[];};struct T{int n;struct S data[];};int main(void){return 0;}' 0 $true
Check 'flex-reject-inferred-array' 'struct S{int n;char data[];};struct S a[]={{1}};int main(void){return 0;}' 0 $true
