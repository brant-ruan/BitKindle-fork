Check 'designated-array-local' 'int main(void){int a[5]={[3]=42};return a[0]||a[1]||a[2]||a[3]!=42||a[4];}' 0
Check 'designated-array-global' 'int a[5]={[3]=42};int main(void){return a[0]||a[3]!=42||a[4];}' 0
Check 'designated-array-inferred' 'int a[]={[4]=42,[1]=3};int main(void){return sizeof(a)!=20||a[4]!=42||a[1]!=3||a[2];}' 0
Check 'designated-array-inferred-local' 'int main(void){int a[]={[4]=42,[1]=3};return sizeof(a)!=20||a[4]!=42||a[1]!=3||a[2];}' 0
Check 'designated-array-continuation' 'int main(void){int a[5]={[2]=20,22};return a[0]||a[1]||a[2]+a[3]!=42||a[4];}' 0
Check 'designated-array-override' 'int main(void){int a[2]={1,2,[0]=42};return a[0]!=42||a[1]!=2;}' 0
Check 'designated-array-ice' 'enum E{N=3};int main(void){int a[5]={[N-1]=42};return a[2]!=42;}' 0
Check 'designated-array-nested-braces' 'int main(void){int a[3][2]={[2]={[1]=42}};return a[0][0]||a[2][0]||a[2][1]!=42;}' 0
Check 'designated-array-struct-elements' 'struct S{int n;};int main(void){struct S a[]={[3]={42},[1]={2}};return sizeof(a)!=16||a[3].n!=42||a[2].n||a[1].n!=2;}' 0
Check 'designated-array-out-of-range' 'int a[2]={[2]=1};int main(void){return 0;}' 0 $true
Check 'designated-array-negative' 'int a[2]={[-1]=1};int main(void){return 0;}' 0 $true
Check 'designated-array-nonconstant' 'int main(void){int n=1;int a[2]={[n]=1};return 0;}' 0 $true
Check 'designated-array-no-equals' 'int a[2]={[1] 42};int main(void){return 0;}' 0 $true
Check 'designated-member-local' 'struct S{int a,b,c;};int main(void){struct S s={.c=42};return s.a||s.b||s.c!=42;}' 0
Check 'designated-member-global' 'struct S{int a,b,c;};struct S s={.c=22,.a=20};int main(void){return s.a+s.c!=42||s.b;}' 0
Check 'designated-member-continuation' 'struct S{int a,b,c;};int main(void){struct S s={.b=20,22};return s.a||s.b+s.c!=42;}' 0
Check 'designated-member-override' 'struct S{int a,b;};int main(void){struct S s={1,2,.a=42};return s.a!=42||s.b!=2;}' 0
Check 'designated-member-nested' 'struct T{int a,b;};struct S{int a;struct T t;};int main(void){struct S s={.t={.b=42}};return s.a||s.t.a||s.t.b!=42;}' 0
Check 'designated-member-array' 'struct S{int a[3];int b;};int main(void){struct S s={.a={[2]=42}};return s.a[0]||s.a[1]||s.a[2]!=42||s.b;}' 0
Check 'designated-member-bitfield' 'struct S{unsigned a:3;unsigned b:7;};int main(void){struct S s={.b=42,.a=3};return s.b!=42||s.a!=3;}' 0
Check 'designated-union-member' 'union U{int a;long long b;};union U u={.b=4294967338LL};int main(void){return u.b!=4294967338LL;}' 0
Check 'designated-record-array-inferred' 'struct S{int a,b;};struct S s[]={{.b=20},{.a=22}};int main(void){return sizeof(s)!=16||s[0].a||s[1].b||s[0].b+s[1].a!=42;}' 0
Check 'designated-member-unknown' 'struct S{int a;};struct S s={.b=1};int main(void){return 0;}' 0 $true
Check 'designated-member-flexible' 'struct S{int n;int a[];};struct S s={.a={1}};int main(void){return 0;}' 0 $true
Check 'designated-member-no-equals' 'struct S{int a;};struct S s={.a 1};int main(void){return 0;}' 0 $true
Check 'designated-member-on-scalar' 'int n={.a=1};int main(void){return 0;}' 0 $true
Check 'designated-aggregate-override-zero' 'struct T{int a,b;};struct S{struct T t;};int main(void){struct S s={.t={1,2},.t={42}};return s.t.a!=42||s.t.b;}' 0
Check 'designated-array-aggregate-override-zero' 'int a[2][2]={[1]={1,2},[1]={42}};int main(void){return a[1][0]!=42||a[1][1];}' 0
Check 'designated-string-override-zero' 'struct S{char a[5];};int main(void){struct S s={.a="abcd",.a="x"};return s.a[0]!=120||s.a[1]||s.a[2]||s.a[3]||s.a[4];}' 0
Check 'designated-chain-array' 'int a[3][3]={[1][1]=20,22};int main(void){return a[1][1]+a[1][2]!=42||a[1][0]||a[2][0];}' 0
Check 'designated-chain-member' 'struct T{int a,b;};struct S{struct T t;int c;};int main(void){struct S s={.t.b=20,22};return s.t.a||s.t.b+s.c!=42;}' 0
Check 'designated-chain-mixed' 'struct T{int a,b;};struct S{struct T t[2];};int main(void){struct S s={.t[1].a=20,22};return s.t[0].a||s.t[1].a+s.t[1].b!=42;}' 0
Check 'designated-chain-return-parent' 'int a[3][3]={[1][1]=20,[2][0]=22};int main(void){return a[1][1]+a[2][0]!=42||a[1][2];}' 0
Check 'designated-chain-preserve-sibling' 'struct T{int a,b;};struct S{struct T t;};int main(void){struct S s={.t={20,1},.t.b=22};return s.t.a+s.t.b!=42;}' 0
Check 'designated-chain-inferred' 'int a[][2]={[4][1]=42,[1][0]=2};int main(void){return sizeof(a)!=40||a[4][1]!=42||a[1][0]!=2||a[3][1];}' 0
Check 'designated-elided-return-parent' 'int main(void){int a[3][3]={[0]=1,2,[2]=42};return a[0][0]!=1||a[0][1]!=2||a[0][2]||a[2][0]!=42;}' 0
Check 'designated-elided-member-parent' 'struct T{int a,b;};struct S{struct T t;int c;};int main(void){struct S s={.t=20,.c=22};return s.t.a+s.c!=42||s.t.b;}' 0
Check 'designated-chain-inferred-records' 'struct S{int a[2];int b;};int main(void){struct S s[]={[3].a[1]=20,22};return sizeof(s)!=48||s[3].a[0]||s[3].a[1]+s[3].b!=42;}' 0
Check 'designated-chain-bitfield' 'struct T{unsigned a:3;unsigned b:7;};struct S{struct T t;};int main(void){struct S s={.t.b=42,.t.a=3};return s.t.b!=42||s.t.a!=3;}' 0
Check 'designated-chain-union' 'struct T{int a,b;};union U{int n;struct T t;};int main(void){union U u={.t.b=42};return u.t.a||u.t.b!=42;}' 0
Check 'designated-chain-invalid-inner-index' 'int a[2][2]={[0][2]=42};int main(void){return 0;}' 0 $true
Check 'designated-chain-invalid-inner-member' 'struct T{int a;};struct S{struct T t;};struct S s={.t.b=42};int main(void){return 0;}' 0 $true
Check 'designated-chain-through-scalar' 'struct S{int a;};struct S s={.a[0]=42};int main(void){return 0;}' 0 $true
