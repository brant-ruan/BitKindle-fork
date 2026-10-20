$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $runtime = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../lib/runtime.c'))
    $prefix = "#include <stdlib.h>`n#include <limits.h>`n"
    $suffix = "`n#include `"$runtime`"`n"
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        Check "integer-lib-$mode-rand-default" ($prefix + 'int expected[]={16838,5758,10113,17515,31051};int main(void){int i;for(i=0;i<5;i++)if(rand()!=expected[i])return 1;srand(1);for(i=0;i<5;i++)if(rand()!=expected[i])return 2;return 0;}' + $suffix)
        Check "integer-lib-$mode-rand-repeat" ($prefix + 'int main(void){unsigned int seeds[]={0U,1U,12345U,UINT_MAX};int history[256],i,j;for(j=0;j<4;j++){srand(seeds[j]);for(i=0;i<256;i++)history[i]=rand();srand(seeds[j]);for(i=0;i<256;i++)if(rand()!=history[i])return 1;}return 0;}' + $suffix)
        Check "integer-lib-$mode-rand-range" ($prefix + 'int main(void){int i,n,lo=RAND_MAX,hi=0;srand(7);for(i=0;i<10000;i++){n=rand();if(n<0||n>RAND_MAX)return 1;if(n<lo)lo=n;if(n>hi)hi=n;}return RAND_MAX<32767||lo==hi;}' + $suffix)
        Check "integer-lib-$mode-rand-pointer-once" ($prefix + 'int main(void){void (*seed)(unsigned int)=srand;int (*next)(void)=rand;unsigned int n=1;seed(n++);return n==2&&next()==16838;}' + $suffix) 1
        foreach ($entry in @(@('abs', 'int', 'INT_MAX'), @('labs', 'long', 'LONG_MAX'), @('llabs', 'long long', 'LLONG_MAX'))) {
            $fn = $entry[0]
            $type = $entry[1]
            $maximum = $entry[2]
            $source = $prefix + "int main(void){$type (*f)($type)=$fn;if(f(0)!=0||$fn(1)!=1||f(-1)!=1)return 1;if($fn($maximum)!=$maximum||f(-$maximum)!=$maximum)return 2;return 0;}" + $suffix
            Check "integer-lib-$mode-$fn-boundaries" $source
            Check "integer-lib-$mode-$fn-once" ($prefix + "int main(void){$type n=-3;$type r=$fn(n++);return r==3&&n==-2;}" + $suffix) 1
        }
        foreach ($entry in @(@('div', 'int', 'div_t', 'INT_MIN', 'INT_MAX'), @('ldiv', 'long', 'ldiv_t', 'LONG_MIN', 'LONG_MAX'), @('lldiv', 'long long', 'lldiv_t', 'LLONG_MIN', 'LLONG_MAX'))) {
            $fn = $entry[0]
            $type = $entry[1]
            $result = $entry[2]
            $minimum = $entry[3]
            $maximum = $entry[4]
            $rows = @('7,3,2,1', '-7,3,-2,-1', '7,-3,-2,1', '-7,-3,2,-1', '0,-7,0,0', '3,7,0,3', '-3,7,0,-3', "$minimum,1,$minimum,0", "$minimum,$minimum,1,0", "$maximum,$minimum,0,$maximum", "$minimum,$maximum,-1,-1")
            if ($type -eq 'long long') {
                $rows += @('LLONG_MIN,3,-3074457345618258602LL,-2', 'LLONG_MAX,-3,-3074457345618258602LL,1', '4294967297LL,2,2147483648LL,1')
            }
            else {
                $rows += @("$minimum,3,-715827882,-2", "$maximum,-3,-715827882,1")
            }
            $table = ($rows | ForEach-Object { '{' + $_ + '}' }) -join ','
            $source = $prefix + "$type cases[][4]={$table};int main(void){int i;$result r;$result (*f)($type,$type)=$fn;for(i=0;i<sizeof cases/sizeof cases[0];i++){r=f(cases[i][0],cases[i][1]);if(r.quot!=cases[i][2]||r.rem!=cases[i][3])return 1;r=$fn(cases[i][0],cases[i][1]);if(r.quot!=cases[i][2]||r.rem!=cases[i][3])return 2;}return 0;}" + $suffix
            Check "integer-lib-$mode-$fn-signs-boundaries" $source
            $source = $prefix + "int main(void){int n,d;$result r;for(n=-64;n<=64;n++){for(d=-16;d<=16;d++){if(d){r=$fn(n,d);if(r.quot*d+r.rem!=n)return 1;if(r.rem>=abs(d)||r.rem<=-abs(d))return 2;if(r.rem&&((r.rem<0)!=(n<0)))return 3;}}}return 0;}" + $suffix
            Check "integer-lib-$mode-$fn-identities" $source
            Check "integer-lib-$mode-$fn-once" ($prefix + "int main(void){int n=7,d=3;$result r=$fn(n++,d++);return r.quot==2&&r.rem==1&&n==8&&d==4;}" + $suffix) 1
        }
    }
    Check 'integer-lib-structure-layout' ($prefix + 'int main(void){return sizeof(div_t)==8&&sizeof(ldiv_t)==8&&sizeof(lldiv_t)==16&&offsetof(div_t,rem)==4&&offsetof(ldiv_t,rem)==4&&offsetof(lldiv_t,rem)==8;}' + $suffix) 1
    Check 'integer-lib-repeat-headers' ($prefix + "#include <stdlib.h>`n#include <stddef.h>`n" + 'int main(void){size_t n=sizeof(div_t);return n==8&&EXIT_SUCCESS==0&&EXIT_FAILURE!=0;}' + $suffix) 1
    Check 'integer-lib-combined-ctype' ($prefix + "#include <ctype.h>`n" + 'int main(void){div_t r=div(-7,3);return isdigit(''0''+abs(r.rem))&&toupper(''a'')==''A'';}' + $suffix) 1
    Check 'integer-lib-return-through-call' ($prefix + 'lldiv_t relay(lldiv_t x){return x;}int main(void){lldiv_t r=relay(lldiv(-4294967297LL,2));return r.quot==-2147483648LL&&r.rem==-1;}' + $suffix) 1
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
