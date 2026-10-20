Check 'infer-local-int' 'int main(void){int a[]={1,2,7};return sizeof a+a[2];}' 19
Check 'infer-global-int' 'int a[]={1,2,7};int main(void){return sizeof a+a[2];}' 19
Check 'infer-static-local' 'int f(void){static int a[]={1,2};return ++a[1];}int main(void){f();return f();}' 4
Check 'infer-static-file' 'static int a[]={2,3};int main(void){return sizeof a+a[1];}' 11
Check 'infer-extern-definition' 'extern int a[]={2,3};int main(void){return sizeof a;}' 8
Check 'infer-after-extern' 'extern int a[];int f(void){return a[1];}int a[]={2,7};int main(void){return f();}' 7
Check 'infer-after-tentative' 'int a[];int a[]={2,7};int main(void){return sizeof a+a[1];}' 15
Check 'infer-char-list' 'int main(void){char a[]={65,66,0};return sizeof a+a[1];}' 69
Check 'infer-wide-list' 'unsigned long long a[]={0xffffffffffffffffULL,7};int main(void){return sizeof a+a[1];}' 23
Check 'infer-pointer-list' 'int n=7;int *a[]={&n,0};int main(void){return sizeof a+*a[0];}' 23
Check 'infer-string-pointers' 'char *a[]={"ab","cd"};int main(void){return sizeof a+a[1][1];}' 116
Check 'infer-local-side-effects' 'int n;int f(void){return ++n;}int main(void){int a[]={f(),f(),f()};return n*10+a[2];}' 33
Check 'infer-comma-expression' 'int main(void){int n=0;int a[]={(n++,7),3};return sizeof a+n+a[0];}' 16
Check 'infer-conditional-comma' 'int main(void){int a[]={1?2,3:4,7};return sizeof a+a[0];}' 11
Check 'infer-nested-conditional' 'int main(void){int a[]={1?(0?2:3),4:5,7};return sizeof a+a[0];}' 12
Check 'infer-call-commas' 'int f(int a,int b){return a+b;}int main(void){int a[]={f(2,3),7};return sizeof a+a[0];}' 13
Check 'infer-sizeof-type' 'int main(void){int a[]={sizeof(int[2]),7};return sizeof a+a[0];}' 16
Check 'infer-self-address-sizeof' 'int main(void){int a[]={sizeof &a,7};return sizeof a+a[0];}' 16
Check 'infer-matrix-braces' 'int a[][2]={{1,2},{7}};int main(void){return sizeof a+a[1][0]+a[1][1];}' 23
Check 'infer-matrix-flat' 'int main(void){int a[][2]={1,2,3};return sizeof a+a[1][0]+a[1][1];}' 19
Check 'infer-matrix-mixed' 'int main(void){int a[][2]={{1},2,3,{7}};return sizeof a+a[2][0]+a[2][1];}' 31
Check 'infer-cube-flat' 'int a[][2][2]={1,2,3,4,5};int main(void){return sizeof a+a[1][0][0]+a[1][1][1];}' 37
Check 'infer-string-rows' 'char a[][4]={"a","bc"};int main(void){return sizeof a+a[1][1]+a[0][3];}' 107
Check 'infer-string-rows-braces' 'int main(void){char a[][4]={{"a"},{"bc"}};return sizeof a+a[1][1];}' 107
Check 'infer-typedef' 'typedef int A[];int main(void){A a={1,2},b={3,4,5};return sizeof a+sizeof b;}' 20
Check 'infer-register' 'int main(void){register int a[]={1,2,3};return sizeof a;}' 12
Check 'infer-for' 'int main(void){for(int a[]={1,7};a[0];a[0]=0){return sizeof a+a[1];}return 0;}' 15
Check 'infer-trailing-comma' 'int a[]={1,2,};int main(void){return sizeof a;}' 8
Check 'infer-scalar-braces' 'int a[]={{1},{{2}}};int main(void){return sizeof a+a[1];}' 10
Check 'infer-completed-next-declarator' 'int main(void){int a[]={1,2},b[sizeof a];return sizeof b;}' 32
Check 'infer-self-sizeof-invalid' 'int main(void){int a[]={sizeof a};return 0;}' 0 $true
Check 'infer-global-self-sizeof-invalid' 'int a[]={sizeof a};int main(void){return 0;}' 0 $true
Check 'infer-static-self-sizeof-invalid' 'int main(void){static int a[]={sizeof a};return 0;}' 0 $true
Check 'infer-typedef-remains-incomplete' 'typedef int A[];int main(void){A a={1,2};return sizeof(A);}' 0 $true
Check 'infer-empty-invalid' 'int a[]={};int main(void){return 0;}' 0 $true
Check 'infer-no-braces-invalid' 'int a[]=7;int main(void){return 0;}' 0 $true
Check 'infer-missing-element-invalid' 'int a[]={1,,2};int main(void){return 0;}' 0 $true
Check 'infer-missing-close-invalid' 'int a[]={1,2;int main(void){return 0;}' 0 $true
Check 'infer-incompatible-prior-bound' 'extern int a[1];int a[]={1,2};int main(void){return 0;}' 0 $true
Check 'infer-too-many-row-elements' 'int a[][2]={{1,2,3}};int main(void){return 0;}' 0 $true
