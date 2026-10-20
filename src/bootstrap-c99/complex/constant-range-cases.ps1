. (Join-Path $PSScriptRoot 'arithmetic-cases.ps1')
$positive = [ordered]@{
    finite = 'double _Complex a=(0x1p512+0x1p510*__c1_i)*(0x1p512+0x1p510*__c1_i);int main(void){double _Complex b=z(0x1p512,0x1p510),c=b*b;return a!=c||(double)a!=0x1.ep1023||im(a)!=0x1p1023;}'
    floatfinite = 'float _Complex a=(0x1p64f+0x1p62f*__c1_i)*(0x1p64f+0x1p62f*__c1_i);int main(void){float _Complex b=(float _Complex)z(0x1p64,0x1p62),c=b*b;return a!=c||(float)a!=0x1.ep127f||im(a)!=0x1p127;}'
    longfinite = 'long double _Complex a=(0x1p512L+0x1p510L*__c1_i)*(0x1p512L+0x1p510L*__c1_i);int main(void){long double _Complex b=(long double _Complex)z(0x1p512,0x1p510),c=b*b;return a!=c||(long double)a!=0x1.ep1023L||im(a)!=0x1p1023;}'
    underflow = 'double _Complex a=(0x1p-537+0x1p-537*__c1_i)*(0x1p-538+0x1p-538*__c1_i);int main(void){return (double)a!=0||im(a)!=0x1p-1074;}'
    floatunderflow = 'float _Complex a=(0x1p-75f+0x1p-75f*__c1_i)*(0x1p-75f+0x1p-75f*__c1_i);int main(void){return (float)a!=0||im(a)!=0x1p-149;}'
    skipped = 'double _Complex a=1?2:(0x1p1023+0x1p1023*__c1_i)*(0x1p1023+0x1p1023*__c1_i);int main(void){return a!=2;}'
}
$expressions = @()
$expected = @()
foreach ($a in @(-1074, -1022, -538, -537, -1, 0, 1, 510, 511, 512)) {
    foreach ($b in @(-1074, -1022, -538, -537, -1, 0, 1, 510, 511, 512)) {
        $e = $a + $b + 1
        if ($e -gt 1023) {
            continue
        }
        $expressions += "((0x1p$a+0x1p$a*__c1_i)*(0x1p$b-0x1p$b*__c1_i))"
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
foreach ($n in @($positive.Keys)) {
    $positive[$n] = $helpers + $positive[$n]
}
$negative = [ordered]@{
    overflow = 'double _Complex a=(0x1p1023+0x1p1023*__c1_i)*(0x1p1023+0x1p1023*__c1_i);int main(void){return 0;}'
    floatoverflow = 'float _Complex a=(0x1p127f+0x1p127f*__c1_i)*(0x1p127f+0x1p127f*__c1_i);int main(void){return 0;}'
}

