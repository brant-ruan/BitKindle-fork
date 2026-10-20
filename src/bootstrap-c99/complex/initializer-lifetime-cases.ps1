. (Join-Path $PSScriptRoot 'constant-divide-cases.ps1')
$grid = $positive['staticruntimegrid']
$positive = [ordered]@{largecomplex = $grid }
$values = (0..299 | ForEach-Object { "f($_).x" }) -join ','
$positive['recordreturns'] = 'struct R{int x;int pad[63];};struct R f(int x){struct R r={0,{0}};r.x=x;return r;}int main(void){int a[]={' + $values + '};int i;for(i=0;i<300;i++)if(a[i]!=i)return 1;return 0;}'
$positive['literals'] = $helpers + 'int main(void){int *p[]={&(int){11},&(int){22},&(int){33}};double _Complex a[]={z(3,4)/z(1,2),z(5,6)/z(2,1)};int q[3]={7,8,9};return *p[0]!=11||*p[1]!=22||*p[2]!=33||p[0]==p[1]||q[2]!=9||im(a[0])>-0.3;}'
$positive['escaped'] = $helpers + 'int main(void){int *p;double _Complex a[]={(p=&(int){19},z(3,4)/z(1,2)),z(5,6)/z(2,1),z(7,8)/z(3,1)};return *p!=19||im(a[0])>-0.3;}'
$positive['nested'] = $helpers + 'struct P{int *p;double _Complex v;};int main(void){struct P a[]={{&(int){13},z(3,4)/z(1,2)},{&(int){27},z(5,6)/z(2,1)}};int x=99;return *a[0].p!=13||*a[1].p!=27||a[0].p==a[1].p||x!=99;}'
$negative = [ordered]@{
    invalidbrace = 'int main(void){int a[]={1{2}};return 0;}'
    missingbrace = 'int main(void){int *p[]={&(int){1},&(int){2};return 0;}'
}



