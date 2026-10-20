. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$positive = [ordered]@{
    large = 'double _Complex a=(0x1p600+0x1p600*__c1_i)/(0x1p600-0x1p600*__c1_i);int main(void){return (double)a!=0||im(a)!=1;}'
    small = 'double _Complex a=(0x1p-600+0x1p-600*__c1_i)/(0x1p-600-0x1p-600*__c1_i);int main(void){return (double)a!=0||im(a)!=1;}'
    subnormal = 'double _Complex a=(0x1p-1074+0x1p-1074*__c1_i)/(0x1p-1074+0x1p-1073*__c1_i);int main(void){return (double)a!=0.6||im(a)!=-0.2;}'
    floatlarge = 'float _Complex a=(0x1p100f+0x1p100f*__c1_i)/(0x1p100f-0x1p100f*__c1_i);int main(void){return (float)a!=0||im(a)!=1;}'
    floatsmall = 'float _Complex a=(0x1p-100f+0x1p-100f*__c1_i)/(0x1p-100f-0x1p-100f*__c1_i);int main(void){return (float)a!=0||im(a)!=1;}'
    longlarge = 'long double _Complex a=(0x1p600L+0x1p600L*__c1_i)/(0x1p600L-0x1p600L*__c1_i);int main(void){return (long double)a!=0||im(a)!=1;}'
    numerator = 'double _Complex a=(0x1.fffffffffffffp1023+0x1.fffffffffffffp1023*__c1_i)/(1.0+__c1_i);int main(void){return (double)a!=0x1.fffffffffffffp1023||im(a)!=0;}'
    imaginaryaxis = 'double _Complex a=(0x1p-1074+0x1p-1074*__c1_i)/(0x1p-1074*__c1_i);int main(void){return (double)a!=1||im(a)!=-1;}'
    skipped = 'double _Complex a=1?2:(0x1p1023+0x1p1023*__c1_i)/(0x1p-1074+0x1p-1074*__c1_i);int main(void){return a!=2;}'
}
$expressions = @()
$expected = @()
foreach ($a in @(-1074, -1022, -600, -538, -1, 0, 1, 510, 600, 1023)) {
    foreach ($b in @(-1074, -1022, -600, -538, -1, 0, 1, 510, 600, 1023)) {
        $e = $a - $b
        if ($e -gt 1023) {
            continue
        }
        $expressions += "((0x1p$a+0x1p$a*__c1_i)/(0x1p$b+0x1p$b*__c1_i))"
        $expected += $(if ($e -lt -1074) {
                '0'
            }
            else {
                "0x1p$e"
            })
    }
}
$initializers = $expressions -join ','
$positive['staticruntimegrid'] = "double _Complex a[]={$initializers};double expected[]={" + ($expected -join ',') + "};int main(void){double _Complex b[]={$initializers};int i;for(i=0;i<sizeof a/sizeof a[0];i++)if(a[i]!=b[i]||(double)a[i]!=expected[i]||im(a[i])!=0)return 1;return 0;}"
$positive['signedaxis'] = 'double _Complex a=(double _Complex)0/(2.0*__c1_i);int main(void){unsigned long long b[2];unsigned char *s=(unsigned char*)&a,*d=(unsigned char*)b;int k;for(k=0;k<16;k++)d[k]=s[k];return b[0]!=0||b[1]!=0x8000000000000000ULL;}'
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $positive[$n]
}
$negative = [ordered]@{
    overflow = 'double _Complex a=(0x1p1023+0x1p1023*__c1_i)/(0x1p-1074+0x1p-1074*__c1_i);int main(void){return 0;}'
    zero = 'double _Complex a=(1.0+__c1_i)/(double _Complex)0;int main(void){return 0;}'
}

