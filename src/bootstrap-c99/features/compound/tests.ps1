Check 'compound-scalar' 'int main(void){return (int){42}!=42;}' 0
Check 'compound-scalar-address' 'int main(void){int *p=&(int){20};*p+=22;return *p!=42;}' 0
Check 'compound-struct-member' 'struct S{int a,b;};int main(void){return (struct S){.b=42}.b!=42;}' 0
Check 'compound-array-subscript' 'int main(void){return (int[3]){1,2,42}[2]!=42;}' 0
Check 'compound-array-inferred' 'int main(void){int *p=(int[]){[3]=42};return p[0]||p[3]!=42;}' 0
Check 'compound-function-argument' 'struct S{int a,b;};int f(struct S s){return s.a+s.b;}int main(void){return f((struct S){20,22})!=42;}' 0
Check 'compound-struct-copy' 'struct S{int a,b;};int main(void){struct S s=(struct S){20,22};return s.a+s.b!=42;}' 0
Check 'compound-distinct-addresses' 'int main(void){int *a=&(int){1},*b=&(int){1};return a==b;}' 0
Check 'compound-repeat-same-block' 'int main(void){int *p=0,*q;int n=0;again:q=&(int){++n};if(p&&p!=q)return 1;p=q;if(n<2)goto again;return *p!=2;}' 0
Check 'compound-nested' 'struct S{int a,b;};int main(void){struct S s=(struct S){(int){20},(int){22}};return s.a+s.b!=42;}' 0
Check 'compound-const-write-rejected' 'int main(void){(const int){1}=2;return 0;}' 0 $true
Check 'compound-void-rejected' 'int main(void){(void){1};return 0;}' 0 $true
Check 'compound-sizeof-parenthesized-array' 'int main(void){return sizeof((int[]){1,2,3})!=12;}' 0
Check 'compound-sizeof-no-side-effect' 'int main(void){int n=0;return sizeof((int){++n})!=4||n;}' 0
Check 'compound-postincrement' 'int main(void){return (int){42}++!=42;}' 0
Check 'compound-sizeof-direct-scalar' 'int main(void){return sizeof(int){42}!=4;}' 0
Check 'compound-sizeof-direct-array' 'int main(void){return sizeof(int[]){1,2,3}!=12;}' 0
Check 'compound-sizeof-direct-side-effect' 'int main(void){int n=0;return sizeof(int){++n}!=4||n;}' 0
Check 'compound-sizeof-direct-member' 'struct S{int a;long long b;};int main(void){return sizeof(struct S){1,2}.b!=8;}' 0
Check 'compound-sizeof-direct-subscript' 'int main(void){return sizeof(long long[]){20,22}[1]!=8;}' 0
Check 'compound-sizeof-direct-bitfield-rejected' 'struct S{unsigned a:3;};int main(void){return sizeof(struct S){1}.a;}' 0 $true
Check 'compound-sizeof-direct-call-not-executed' 'int n;int f(void){n++;return 42;}int main(void){return sizeof(int){f()}!=4||n;}' 0
Check 'compound-file-scalar-address' 'int *p=&(int){42};int main(void){return *p!=42;}' 0
Check 'compound-file-array' 'int *p=(int[]){20,22};int main(void){return p[0]+p[1]!=42;}' 0
Check 'compound-file-member-address' 'struct S{int a,b;};int *p=&(struct S){20,42}.b;int main(void){return *p!=42;}' 0
Check 'compound-file-nested-address' 'struct S{int *p;};struct S *p=&(struct S){(int[]){42}};int main(void){return *p->p!=42;}' 0
Check 'compound-file-distinct' 'int *a=&(int){42},*b=&(int){42};int main(void){return a==b||*a!=42||*b!=42;}' 0
Check 'compound-file-mutable' 'int *p=&(int){20};int main(void){*p+=22;return *p!=42;}' 0
Check 'compound-block-static-address-rejected' 'int main(void){static int *p=&(int){42};return 0;}' 0 $true
Check 'compound-file-nonconstant-rejected' 'int f(void){return 42;}int *p=&(int){f()};int main(void){return 0;}' 0 $true
Check 'compound-sizeof-file-array' 'unsigned long long n=sizeof(int[]){1,2,3};int main(void){return n!=12;}' 0
Check 'compound-sizeof-file-member' 'struct S{int a;long long b;};int n=sizeof(struct S){1,2}.b;int main(void){return n!=8;}' 0
Check 'compound-sizeof-enum-bound' 'enum E{N=sizeof((int[]){1,2,3})};int a[N];int main(void){return N!=12||sizeof(a)!=48;}' 0
Check 'compound-sizeof-file-nested' 'struct S{int *p;};int n=sizeof(struct S){(int[]){42}};int *p=&(int){42};int main(void){return n!=8||*p!=42;}' 0
Check 'compound-inferred-record-copy' 'struct S{int a,b;};int main(void){struct S s={20,22};struct S *p=(struct S[]){s,s};return p[0].a+p[1].b!=42;}' 0
Check 'compound-inferred-record-function' 'struct S{int a,b;};int n;struct S f(void){struct S s={20,22};n++;return s;}int main(void){struct S *p=(struct S[]){f(),f()};return n!=2||p[0].a+p[1].b!=42;}' 0
Check 'compound-inferred-record-size' 'struct S{int a,b;};int main(void){struct S s={20,22};return sizeof((struct S[]){s,s,s})!=24;}' 0
Check 'compound-inferred-nested-probes' 'struct S{int a,b;};int main(void){struct S s={20,22};struct S *p=(struct S[]){(struct S[]){s,s}[1],s};return p[0].a+p[1].b!=42;}' 0
