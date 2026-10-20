param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$Implemented = $true
$CaseNames = $null
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$CompilerDirectory = Join-Path $BuildDirectory 'host'
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath('src/bootstrap-c99')
$CompilerDirectory = [IO.Path]::GetFullPath($CompilerDirectory)
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'")).Replace("'^C1 error: '", "'(?m)^C1 error: '")
$cases = [ordered]@{
    local = 'int main(void){int n=3;int a[n];a[0]=4;a[2]=7;return sizeof a!=12||a[0]+a[2]!=11;}'
    capture = 'int main(void){int n=3;int a[n++];n=9;return sizeof a!=12||n!=9;}'
    matrix = 'int main(void){int n=2,m=3;int a[n][m];a[1][2]=7;return sizeof a!=24||sizeof a[0]!=12||a[1][2]!=7;}'
    typedef = 'int main(void){int n=3;typedef int A[n++];A a;A b;n=8;a[2]=4;b[2]=7;return sizeof a!=12||sizeof b!=12||a[2]+b[2]!=11;}'
    pointer = 'int main(void){int n=3;int a[2][3]={{1,2,3},{4,5,7}};int (*p)[n]=a;return p[1][2]!=7||sizeof *p!=12||(p+1)-p!=1;}'
    parameter = 'int f(int n,int a[n]){return a[n-1];}int main(void){int a[3]={1,2,7};return f(3,a)!=7;}'
    parameter_matrix = 'int f(int n,int m,int a[n][m]){return a[n-1][m-1]+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}};return f(2,3,a)!=19;}'
    loop = 'int main(void){int i,s=0;for(i=1;i<20;i++){int a[i];a[i-1]=i;if(i%2)continue;s+=a[i-1];}return s!=90;}'
    shadow = 'int main(void){int n=3,s;int a[n];a[2]=7;{int n=5;int a[n];a[4]=4;s=a[4]+sizeof a;}return s!=24||a[2]!=7||sizeof a!=12;}'
    sizeof_type = 'int main(void){int n=3;unsigned long long s=sizeof(int[n++]);return s!=12||n!=4;}'
    sizeof_matrix = 'int main(void){int n=2,m=3;return sizeof(int[n][m])!=24;}'
    sizeof_typedef = 'int main(void){int n=3;typedef int A[n++];n=9;return sizeof(A)!=12||n!=9;}'
    sizeof_pointer = 'int main(void){int n=3;int a[3];int (*p)[n]=&a;n=5;return sizeof *p!=12;}'
    sizeof_call = 'int calls;int bound(char*s){calls++;return s[0]-117;}int main(void){unsigned long long s=sizeof(int[bound("x")]);return s!=12||calls!=1;}'
    sizeof_wide = 'int main(void){unsigned long long n=1073741824ULL;return sizeof(int[n])!=4294967296ULL;}'
    pointer_increment = 'int main(void){int n=3;int a[3][3]={{1,2,3},{4,5,6},{7,8,9}};int (*p)[n]=a;int s=(*p++)[2];s+=(*++p)[2];s+=(*p--)[0];s+=(*--p)[0];return s!=20||p!=a;}'
    pointer_compound = 'int main(void){int n=3;int a[3][3]={{1,2,3},{4,5,6},{7,8,9}};int (*p)[n]=a;int (**q)[n]=&p;*q+=2;*q-=1;return (*p)[2]!=6||p-a!=1||a-p!=-1;}'
    pointer_commute = 'int main(void){int n=3;int a[3][3]={{1,2,3},{4,5,6},{7,8,9}};int (*p)[n]=a;return (2+p)[0][2]!=9||(p+2)[-1][1]!=5;}'
    pointer_capture = 'int main(void){int n=3;int a[2][3]={{1,2,3},{4,5,7}};int (*p)[n]=a;n=1;return p[1][2]!=7||(p+1)-p!=1;}'
    storage_pages = 'int main(void){int n=20000,i;int a[n];for(i=0;i<n;i++)a[i]=i;return a[0]!=0||a[n-1]!=19999;}'
    storage_continue = 'int main(void){int i,n=8192,s=0;for(i=0;i<1000;i++){int a[n];a[n-1]=i;if(i%2)continue;s+=a[n-1];}return s!=249500;}'
    storage_break = 'int main(void){int i,n=8192;for(i=0;i<1000;i++){while(1){int a[n];a[n-1]=i;if(a[n-1]!=i)return 1;break;}}return 0;}'
    storage_shadow_continue = 'int main(void){int i,n=8192;for(i=0;i<1000;i++){int a[n];a[0]=i;{int a[n];a[0]=7;if(a[0]==7)continue;}return 1;}return 0;}'
    storage_address = 'int main(void){int n=3;int a[n];int (*p)[n]=&a;a[2]=7;return p[0][2]!=7||sizeof *p!=12;}'
    goto_out = 'int main(void){int i,n=8192;for(i=0;i<1000;i++){{int a[n];a[0]=i;goto out;}out:;}return 0;}'
    goto_back = 'int main(void){int i=0,n=8192;again:;{int a[n];a[0]=i;if(++i<1000)goto again;}return i!=1000;}'
    goto_same = 'int main(void){int n=3,i=0;int a[n];a[2]=7;again:if(++i<5)goto again;return a[2]!=7;}'
    goto_outer = 'int main(void){int n=3,m=8192,i;int a[n];a[2]=7;for(i=0;i<1000;i++){{int a[m];a[0]=i;goto out;}out:;}return a[2]!=7;}'
    goto_before = 'int main(void){int i=0,n=8192;again:;int a[n];a[0]=i;if(++i<1000)goto again;return i!=1000;}'
    parameter_effect = 'int f(int n,int a[n++]){return n+a[0];}int main(void){int a[3]={7,0,0};return f(3,a)!=11;}'
    parameter_call = 'int calls;int bound(int n){calls++;return n;}int f(int n,int a[][bound(n)],int z){return a[1][n-1]+z+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}};return f(3,a,5)!=24||calls!=1;}'
    parameter_stack = 'int f(int x,int y,int z,int w,int n,int a[][n]){return x+y+z+w+a[1][n-1]+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}};return f(1,2,3,4,3,a)!=29;}'
    parameter_parens = 'int (f)(int n,int a[][n]){return a[1][n-1]+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}};return f(3,a)!=19;}'
    parameter_float = 'int f(double x,int n,int a[][n]){return (int)x+a[1][n-1]+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}};return f(5.0,3,a)!=24;}'
    parameter_repeat = 'int f(int n,int a[][n]){return a[1][n-1]+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}},b[2][2]={{1,2},{3,4}};return f(3,a)!=19||f(2,b)!=12;}'
    parameter_old = 'int f(n,a) int n;int a[][n];{return a[1][n-1]+sizeof *a;}int main(void){int a[2][3]={{1,2,3},{4,5,7}};return f(3,a)!=19;}'
    static_pointer = 'int main(void){int n=3;static int (*p)[n];int a[2][3]={{1,2,3},{4,5,7}};p=a;return sizeof *p!=12||p[1][2]!=7;}'
    switch_outer = 'int main(void){int n=3;int a[n];a[2]=7;switch(n){case 3:return a[2]!=7;default:return 1;}}'
    switch_inner = 'int main(void){int n=8192,i;for(i=0;i<1000;i++){switch(i%2){case 0:{int a[n];a[n-1]=7;if(a[n-1]!=7)return 1;break;}default:{int a[n];a[0]=9;if(a[0]!=9)return 2;continue;}}}return 0;}'
    switch_nested = 'int main(void){int n=3,s=0;switch(n){case 3:{int a[n];a[2]=7;switch(a[2]){case 7:s=a[2];break;default:return 1;}break;}default:return 2;}return s!=7;}'
    composite_conditional = 'int main(void){int n=3,m=3,c=1;int a[2][3]={{1,2,3},{4,5,7}};int (*p)[n]=a;int (*q)[m]=a;return (c?p:q)[1][2]!=7||sizeof *(c?p:q)!=12;}'
    qualified_typedef = 'int main(void){int n=3;typedef int A[n];const A a;return sizeof a!=12;}'
    composite_nested = 'int main(void){int n=2,m=3,k=2,l=3;int a[2][2][3]={{{1,2,3},{4,5,6}},{{7,8,9},{10,11,12}}};int (*p)[n][m]=a;int (*q)[k][l]=a;return p!=q||p-q!=0||p[1][1][2]!=12;}'
    sizeof_increment = 'int main(void){int n=3;int a[2][3];int (*p)[n]=a;unsigned long long s=sizeof *p++;return s!=12||p!=a+1;}'
    sizeof_fixed_control = 'int main(void){int a[2][3];int (*p)[3]=a;unsigned long long s=sizeof *p++;return s!=12||p!=a;}'
    sizeof_nested_control = 'int main(void){int n=3;int a[2][3];int (*p)[n]=a;unsigned long long s=sizeof(sizeof *p++);return s!=8||p!=a;}'
    sizeof_operand_call = 'int calls;int f(int a,int b,int c,int d,int e){calls++;return a+b+c+d+e;}int main(void){int n=3,x=17;int a[2][3];int (*p)[n]=a;unsigned long long s=sizeof *(f(1,2,3,4,5),p++);return s!=12||calls!=1||p!=a+1||x!=17;}'
    for_array = 'int main(void){int n=8192,i,j,s=0;for(j=0;j<1000;j++){i=0;for(int a[n];i<4;i++){a[i]=i;if(i==2)continue;s+=a[i];}}return s!=4000;}'
    for_array_break = 'int main(void){int n=8192,i,j;for(j=0;j<1000;j++){i=0;for(int a[n];;i++){a[0]=7;if(i==2)break;}}return i!=2;}'
    function_vm_typedef = 'int main(void){int n=3;typedef int (*F(void))[n];F *p=0;return sizeof p!=8||p!=0;}'
    register_sizeof = 'int main(void){int n=3;register int a[n];return sizeof a!=12;}'
}
$negative = [ordered]@{
    goto_array = 'int main(void){int n=3;goto inside;{int a[n];inside:return 0;}}'
    goto_pointer = 'int main(void){int n=3;goto inside;{int (*p)[n];inside:return 0;}}'
    goto_typedef = 'int main(void){int n=3;goto inside;{typedef int A[n];inside:return 0;}}'
    goto_sibling = 'int main(void){int n=3;{int a[n];goto inside;}{int b[n];inside:return 0;}}'
    goto_forward = 'int main(void){int n=3;goto inside;int a[n];inside:return 0;}'
    initializer = 'int main(void){int n=3;int a[n]={0};return 0;}'
    switch_array = 'int main(void){int n=3;switch(n){int a[n];case 3:return 0;}return 0;}'
    switch_typedef = 'int main(void){int n=3;switch(n){typedef int A[n];default:return 0;}}'
    switch_pointer = 'int main(void){int n=3;switch(n){{int (*p)[n];case 3:return 0;}}return 0;}'
    extern_pointer = 'int main(void){int n=3;extern int (*p)[n];return 0;}'
    extern_array = 'int main(void){int n=3;extern int a[n];return 0;}'
    static_array = 'int main(void){int n=3;static int a[n];return 0;}'
    static_pointer_goto = 'int main(void){int n=3;goto target;{static int (*p)[n];target:return 0;}}'
    qualified_write = 'int main(void){int n=3;typedef int A[n];const A a;a[0]=1;return 0;}'
    sizeof_enum = 'int main(void){int n=3;int a[n];enum E {K=sizeof a};return 0;}'
    sizeof_case = 'int main(void){int n=3;int a[n];switch(n){case sizeof a:return 0;}return 0;}'
    function_vm_linkage = 'int main(void){int n=3;int (*f(void))[n];return 0;}'
    function_vm_typedef_goto = 'int main(void){int n=3;goto target;{typedef int (*F(void))[n];target:return 0;}}'
}
$selection = $CaseNames
if ($selection) {
    foreach ($name in @($cases.Keys)) {
        if ($selection -notcontains $name) {
            $cases.Remove($name)
        }
    }
}
$commands = @()
foreach ($name in $cases.Keys) {
    $commands += "Check '$name' '$($cases[$name])' 0 $(if(!$Implemented){'$true'})"
}
if ($Implemented) {
    foreach ($name in $negative.Keys) {
        $commands += "Check 'reject-$name' '$($negative[$name])' 0 `$true"
    }
}
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = 'c99'
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($g in 1, 2) {
        & ([scriptblock]::Create($prefix + ($commands -join "`n"))) -Compiler "$CompilerDirectory/c1-stage$g.exe" -Assembler "$BuildDirectory/link-stage$g.exe" -CompilerKind C1 -ScratchDirectory "$BuildDirectory/namespace/vla/stage$g"
        if (!$?) {
            throw 'Runtime VLA test failed'
        }
    }
    if ($Implemented) {
        foreach ($name in $cases.Keys) {
            if ((Get-FileHash "$BuildDirectory/namespace/vla/stage1/$name.exe").Hash -ne (Get-FileHash "$BuildDirectory/namespace/vla/stage2/$name.exe").Hash) {
                throw 'Generation mismatch'
            }
        }
    }
    if ($Implemented) {
        "PASS $($cases.Count*2) runtime VLA executions, $($negative.Count*2) constraint rejections and $($cases.Count) equal pairs"
    }
    else {
        "BASELINE: $($cases.Count*2) valid runtime VLA programs rejected; runtime VLA implementation remains required"
    }
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
