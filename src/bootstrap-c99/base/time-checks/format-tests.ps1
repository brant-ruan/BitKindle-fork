
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')
$prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/strftime.c`"`n"
Check 'strftime-basic-fields' ($prefix + @'
int main(void){struct tm v;char b[256];memset(&v,0,sizeof(v));v.tm_year=99;v.tm_mon=0;v.tm_mday=3;v.tm_wday=0;v.tm_yday=2;v.tm_hour=13;v.tm_min=4;v.tm_sec=5;if(!strftime(b,sizeof(b),"%a|%A|%b|%B|%d|%H|%I|%j|%m|%M|%p|%S|%U|%w|%W|%y|%Y|%%",&v))return 1;if(strcmp(b,"Sun|Sunday|Jan|January|03|13|01|003|01|04|PM|05|01|0|00|99|1999|%"))return 2;return 0;}
'@) 0
Check 'strftime-composite-c99' ($prefix + @'
int main(void){struct tm v;char b[256];memset(&v,0,sizeof(v));v.tm_year=99;v.tm_mday=3;v.tm_yday=2;v.tm_hour=13;v.tm_min=4;v.tm_sec=5;strftime(b,sizeof(b),"%c|%x|%X|%C|%D|%e|%F|%h|%r|%R|%T|%u%n%t",&v);return strcmp(b,"Sun Jan  3 13:04:05 1999|01/03/99|13:04:05|19|01/03/99| 3|1999-01-03|Jan|01:04:05 PM|13:04|13:04:05|7\n\t");}
'@) 0
Check 'strftime-c-locale-modifiers' ($prefix + @'
int main(void){time_t t=0;struct tm *v=gmtime(&t);char a[256],b[256];strftime(a,sizeof(a),"%c %C %x %X %y %Y %d %e %H %I %m %M %S %u %U %V %w %W %y",v);strftime(b,sizeof(b),"%Ec %EC %Ex %EX %Ey %EY %Od %Oe %OH %OI %Om %OM %OS %Ou %OU %OV %Ow %OW %Oy",v);return strcmp(a,b);}
'@) 0
Check 'strftime-buffer-boundaries' ($prefix + @'
int main(void){char b[12];int i;time_t t=0;struct tm *v=gmtime(&t);for(i=0;i<=11;i++){memset(b,'!',sizeof(b));if(strftime(b,i,"%F",v)!=(i==11?10:0))return 1;if(b[i]!='!')return 2;if(i==11&&strcmp(b,"1970-01-01"))return 3;}if(strftime(0,0,"%Y",v))return 4;return 0;}
'@) 0
Check 'strftime-empty-no-static-overwrite' ($prefix + @'
int main(void){time_t t=0;struct tm *v=gmtime(&t);char *s=asctime(v),b[32];if(strftime(b,sizeof(b),"",v)||*b)return 1;if(strftime(b,sizeof(b),"%Y",v)!=4||strcmp(b,"1970"))return 2;if(v->tm_year!=70||strcmp(s,"Thu Jan  1 00:00:00 1970\n"))return 3;return 0;}
'@) 0
Check 'strftime-zone-fixture' ($prefix + @'
int main(void){struct __c1_time_output out;struct tm v;char b[80];memset(&out,0,sizeof(out));memset(&v,0,sizeof(v));out.buffer=b;out.capacity=80;out.zone_loaded=1;out.zone.bias=-330;__c1_time_format(&out,"%z %Z",&v,0);b[out.used]=0;if(strcmp(b,"+0530 UTC+0530"))return 1;out.used=0;v.tm_isdst=-1;__c1_time_format(&out,"[%z][%Z]",&v,0);b[out.used]=0;return strcmp(b,"[][]");}
'@) 0
Check 'strftime-invalid-extension' ($prefix + @'
int main(void){time_t t=0;struct tm *v=gmtime(&t);char b[20];if(strftime(b,sizeof(b),"%",v)||strftime(b,sizeof(b),"%Q",v)||strftime(b,sizeof(b),"%Ea",v))return 1;v->tm_wday=99;if(strftime(b,sizeof(b),"%a",v))return 2;return 0;}
'@) 0
$rows = [Collections.Generic.List[string]]::new()
for ($year = 1990; $year -lt 2030; $year++) {
    for ($offset = -2; $offset -le 2; $offset++) {
        $d = ([datetime]::new($year, 1, 1)).AddDays($offset)
        $t = [DateTimeOffset]::new([datetime]::SpecifyKind($d, [DateTimeKind]::Utc)).ToUnixTimeSeconds()
        $isoYear = [C1ISOWeek]::GetYear($d)
        $week = [C1ISOWeek]::GetWeekOfYear($d)
        $day = if ([int]$d.DayOfWeek -eq 0) {
            7
        }
        else {
            [int]$d.DayOfWeek
        }
        $expected = '{0:0000}-W{1:00}-{2}' -f $isoYear, $week, $day
        $rows.Add('{' + $t + 'LL,"' + $expected + '"}')
    }
}
$source = $prefix + 'static struct R{time_t t;char *expected;} rows[]={' + ($rows -join ',') + "};`n" + @'
int main(void){int i;char b[32];for(i=0;i<200;i++){if(strftime(b,sizeof(b),"%G-W%V-%u",gmtime(&rows[i].t))!=10||strcmp(b,rows[i].expected))return 1;}return 0;}
'@
Check 'strftime-iso-independent-oracle-200' $source 0
