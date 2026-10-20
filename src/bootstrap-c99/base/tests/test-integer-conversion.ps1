$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
function ConversionDigits([System.Numerics.BigInteger]$Value, [int]$Base) {
    # Test fixture text only; no compiler or library implementation uses this.
    $digits = '0123456789abcdefghijklmnopqrstuvwxyz'
    $result = ''
    do {
        $remainder = [int]($Value % $Base)
        $result = $digits[$remainder] + $result
        $Value = [System.Numerics.BigInteger]::Divide($Value, [System.Numerics.BigInteger]$Base)
    } while ($Value -gt 0)
    return $result
}
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $runtime = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../lib/runtime.c'))
    $nl = [string][char]10
    $prefix = ('#include <stdlib.h>', '#include <errno.h>', '#include <limits.h>', '#include <string.h>', '') -join $nl
    $suffix = $nl + '#include "' + $runtime + '"' + $nl
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        foreach ($entry in @(@('strtol', 'long', 'LONG_MIN', 'LONG_MAX', '2147483647', '-2147483648', '2147483648', '-2147483649'), @('strtoll', 'long long', 'LLONG_MIN', 'LLONG_MAX', '9223372036854775807', '-9223372036854775808', '9223372036854775808', '-9223372036854775809'))) {
            $fn = $entry[0]
            $type = $entry[1]
            $min = $entry[2]
            $max = $entry[3]
            $maxText = $entry[4]
            $minText = $entry[5]
            $over = $entry[6]
            $under = $entry[7]
            $source = "int main(void){char *e;const char *a=" + [char]34 + $maxText + '!' + [char]34 + ',*b=' + [char]34 + $minText + '!' + [char]34 + ";errno=77;if($fn(a,&e,10)!=$max||*e!=33||errno!=77)return 1;if($fn(b,&e,10)!=$min||*e!=33||errno!=77)return 2;return 0;}"
            Check "strto-$mode-$fn-extremes" ($prefix + $source + $suffix)
            $source = "int main(void){char *e;const char *a=" + [char]34 + $over + 'xyz' + [char]34 + ',*b=' + [char]34 + $under + 'xyz' + [char]34 + ";errno=0;if($fn(a,&e,10)!=$max||errno!=ERANGE||*e!=120)return 1;errno=0;if($fn(b,&e,10)!=$min||errno!=ERANGE||*e!=120)return 2;return 0;}"
            Check "strto-$mode-$fn-overflow" ($prefix + $source + $suffix)
            Check "strto-$mode-$fn-signs" ($prefix + "int main(void){return $fn(" + [char]34 + "-123" + [char]34 + ",NULL,10)!=-123||$fn(" + [char]34 + "+123" + [char]34 + ",NULL,10)!=123||$fn(" + [char]34 + "-0" + [char]34 + ",NULL,10)!=0;}" + $suffix)
        }
        foreach ($entry in @(@('strtoul', 'ULONG_MAX', '4294967295', '4294967296'), @('strtoull', 'ULLONG_MAX', '18446744073709551615', '18446744073709551616'))) {
            $fn = $entry[0]
            $max = $entry[1]
            $maxText = $entry[2]
            $over = $entry[3]
            $source = "int main(void){char *e;errno=77;if($fn(" + [char]34 + $maxText + '!' + [char]34 + ",&e,10)!=$max||errno!=77||*e!=33)return 1;if($fn(" + [char]34 + '-' + $maxText + [char]34 + ",NULL,10)!=1||errno!=77)return 2;if($fn(" + [char]34 + '-1' + [char]34 + ",NULL,10)!=$max||errno!=77)return 3;return 0;}"
            Check "strto-$mode-$fn-extremes-negate" ($prefix + $source + $suffix)
            $source = "int main(void){char *e;errno=0;if($fn(" + [char]34 + $over + '!' + [char]34 + ",&e,10)!=$max||errno!=ERANGE||*e!=33)return 1;errno=0;if($fn(" + [char]34 + '-' + $over + '!' + [char]34 + ",&e,10)!=$max||errno!=ERANGE||*e!=33)return 2;return 0;}"
            Check "strto-$mode-$fn-overflow" ($prefix + $source + $suffix)
        }
        foreach ($fn in 'strtol', 'strtoul', 'strtoll', 'strtoull') {
            $template = @'
int main(void){const char *s[]={""," ","+","-","  +","--1","+-1","x","0x","0Xg","+0x","08","0b10","  -0xg"};
int end[]={0,0,0,0,0,0,0,0,1,1,2,1,1,4};char *p;int i;for(i=0;i<14;i++){errno=77;if(FN(s[i],&p,0)!=0||p!=s[i]+end[i]||errno!=77)return 1;}return 0;}
'@
            Check "strto-$mode-$fn-no-conversion-prefix" ($prefix + $template.Replace('FN', $fn) + $suffix)
            $template = @'
int main(void){const char *s[]={"42!","+42!"," 42!","\t42!","\n42!","\v42!","\f42!","\r42!","0x2a!","0X2A!","052!","00042!","42u","0x"};
int base[]={10,10,10,10,10,10,10,10,0,16,0,10,10,16};int value[]={42,42,42,42,42,42,42,42,42,42,42,42,42,0};
int end[]={2,3,3,3,3,3,3,3,4,4,3,5,2,1};char *p;int i;for(i=0;i<14;i++){errno=77;if(FN(s[i],&p,base[i])!=value[i]||p!=s[i]+end[i]||errno!=77)return 1;}return 0;}
'@
            Check "strto-$mode-$fn-syntax" ($prefix + $template.Replace('FN', $fn) + $suffix)
            $template = @'
int main(void){char digits[]="0123456789abcdefghijklmnopqrstuvwxyz",s[66],*end;unsigned int value,x;int base,n,i;
for(base=2;base<=36;base++)for(value=0;value<4096;value+=17){x=value;n=0;do{s[n++]=digits[x%(unsigned int)base];x/=(unsigned int)base;}while(x);
for(i=0;i<n/2;i++){char c=s[i];s[i]=s[n-i-1];s[n-i-1]=c;}s[n]=33;s[n+1]=0;errno=77;if(FN(s,&end,base)!=value||end!=s+n||errno!=77)return 1;
for(i=0;i<n;i++)if(s[i]>=97)s[i]-=32;if(FN(s,&end,base)!=value||end!=s+n)return 2;}return 0;}
'@
            Check "strto-$mode-$fn-all-bases" ($prefix + $template.Replace('FN', $fn) + $suffix)
            $template = @'
int main(void){int base[]={INT_MIN,-1,1,37,INT_MAX};const char *s="123";char *end;int i;for(i=0;i<5;i++){errno=0;if(FN(s,&end,base[i])!=0||end!=s||errno!=EINVAL)return 1;}return 0;}
'@
            Check "strto-$mode-$fn-invalid-base" ($prefix + $template.Replace('FN', $fn) + $suffix)
            $template = @'
int main(void){char s[2051],*end;int i;for(i=0;i<2048;i++)s[i]=57;s[2048]=33;s[2049]=0;errno=0;FN(s,&end,10);if(errno!=ERANGE||end!=s+2048)return 1;for(i=0;i<2048;i++)s[i]=48;s[2048]=49;s[2049]=33;s[2050]=0;errno=77;return FN(s,&end,10)!=1||end!=s+2049||errno!=77;}
'@
            Check "strto-$mode-$fn-long-input" ($prefix + $template.Replace('FN', $fn) + $suffix)
            $template = @'
int main(void){char a[]={(char)160,49,0},b[]={49,(char)255,0},*e;errno=77;if(FN(a,&e,10)!=0||e!=a||errno!=77)return 1;return FN(b,&e,10)!=1||e!=b+1||errno!=77;}
'@
            Check "strto-$mode-$fn-high-bytes" ($prefix + $template.Replace('FN', $fn) + $suffix)
        }
        foreach ($entry in @(@('strtol', 'long', 'LONG_MAX', 'LONG_MIN', '2147483647', $true), @('strtoll', 'long long', 'LLONG_MAX', 'LLONG_MIN', '9223372036854775807', $true), @('strtoul', 'unsigned long', 'ULONG_MAX', '', '4294967295', $false), @('strtoull', 'unsigned long long', 'ULLONG_MAX', '', '18446744073709551615', $false))) {
            $fn = $entry[0]
            $type = $entry[1]
            $maximum = $entry[2]
            $minimum = $entry[3]
            $magnitude = [System.Numerics.BigInteger]::Parse($entry[4])
            $signed = $entry[5]
            $rows = [Collections.Generic.List[string]]::new()
            foreach ($base in 2..36) {
                $maxDigits = ConversionDigits $magnitude $base
                $overDigits = ConversionDigits ($magnitude + 1) $base
                $rows.Add('{"' + $maxDigits + '!",' + $base + ',' + $maximum + ',77}')
                $rows.Add('{"' + $overDigits + '!",' + $base + ',' + $maximum + ',ERANGE}')
                if ($signed) {
                    $underDigits = ConversionDigits ($magnitude + 2) $base
                    $rows.Add('{"-' + $maxDigits + '!",' + $base + ',-' + $maximum + ',77}')
                    $rows.Add('{"-' + $overDigits + '!",' + $base + ',' + $minimum + ',77}')
                    $rows.Add('{"-' + $underDigits + '!",' + $base + ',' + $minimum + ',ERANGE}')
                }
                else {
                    $rows.Add('{"-' + $maxDigits + '!",' + $base + ',1,77}')
                    $rows.Add('{"-' + $overDigits + '!",' + $base + ',' + $maximum + ',ERANGE}')
                    $rows.Add('{"-1!",' + $base + ',' + $maximum + ',77}')
                }
                $smallDigits = ConversionDigits ($magnitude - 1) $base
                $rows.Add('{"' + $smallDigits + '!",' + $base + ',' + $maximum + '-1,77}')
            }
            $source = 'struct Test {const char *text;int base;' + $type + ' value;int error;};struct Test cases[]={' + ($rows -join ',') + '};int main(void){int i;char *end;for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++){errno=77;if(' + $fn + '(cases[i].text,&end,cases[i].base)!=cases[i].value||errno!=cases[i].error||*end!=33||end!=cases[i].text+strlen(cases[i].text)-1)return 1;}return 0;}'
            Check "strto-$mode-$fn-radix-boundaries" ($prefix + $source + $suffix)
        }
        $cases = @{
            'hex-signed' = 'int main(void){char *e;errno=0;if(strtol("7fffffff",&e,16)!=LONG_MAX||errno)return 1;if(strtol("-80000000",&e,16)!=LONG_MIN||errno)return 2;if(strtoll("7fffffffffffffff",&e,16)!=LLONG_MAX||errno)return 3;if(strtoll("-8000000000000000",&e,16)!=LLONG_MIN||errno)return 4;if(strtoll("8000000000000000",&e,16)!=LLONG_MAX||errno!=ERANGE)return 5;return 0;}'
            'hex-unsigned' = 'int main(void){errno=0;if(strtoul("ffffffff",NULL,16)!=ULONG_MAX||errno)return 1;if(strtoull("ffffffffffffffff",NULL,16)!=ULLONG_MAX||errno)return 2;if(strtoull("10000000000000000",NULL,16)!=ULLONG_MAX||errno!=ERANGE)return 3;return 0;}'
            'binary-boundaries' = 'int main(void){char a[66],*e;int i;for(i=0;i<64;i++)a[i]=49;a[64]=0;errno=0;if(strtoull(a,&e,2)!=ULLONG_MAX||e!=a+64||errno)return 1;a[64]=49;a[65]=0;if(strtoull(a,&e,2)!=ULLONG_MAX||e!=a+65||errno!=ERANGE)return 2;return 0;}'
            'atoi-family' = 'int main(void){errno=77;if(atoi(" \t-123x")!=-123||atol("+2147483647")!=LONG_MAX||atol("-2147483648")!=LONG_MIN)return 1;if(atoll("9223372036854775807")!=LLONG_MAX||atoll("-9223372036854775808")!=LLONG_MIN)return 2;return atoi("0x10")!=0||atoi("010")!=10||atoi("x")!=0||atoll("+")!=0||errno!=77;}'
            'function-pointers' = 'int main(void){long (*a)(const char *,char **,int)=strtol;unsigned long (*b)(const char *,char **,int)=strtoul;long long (*c)(const char *,char **,int)=strtoll;unsigned long long (*d)(const char *,char **,int)=strtoull;int (*e)(const char *)=atoi;long (*f)(const char *)=atol;long long (*g)(const char *)=atoll;int base=10;return a("3",NULL,base++)!=3||base!=11||b("4",NULL,10)!=4||c("5",NULL,10)!=5||d("6",NULL,10)!=6||e("7")!=7||f("8")!=8||g("9")!=9;}'
            'errno-success-preserved' = 'int main(void){errno=ERANGE;if(strtol("7",NULL,10)!=7||errno!=ERANGE)return 1;errno=EDOM;return strtoull("x",NULL,10)!=0||errno!=EDOM;}'
            'invalid-message' = 'int main(void){return strcmp(strerror(EINVAL),"Invalid argument")!=0;}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "strto-$mode-$name" ($prefix + $cases[$name] + $suffix) 
        }
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
