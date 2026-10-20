$prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/strftime.c`"`n#include `"src/bootstrap-c99/base/lib/clock.c`"`n"
$fixture = @'
static struct __c1_dynamic_zone pacific(void){struct __c1_dynamic_zone z;int i;char *key="Pacific Standard Time";memset(&z,0,sizeof(z));z.base.bias=480;for(i=0;key[i];i++)z.key[i]=(unsigned char)key[i];return z;}
'@+ "`n"
Check 'history-layout-and-rules' ($prefix + $fixture + @'
int main(void){struct __c1_dynamic_zone z=pacific();struct __c1_zone a,b;if(sizeof(z)!=432||offsetof(struct __c1_dynamic_zone,key)!=172||offsetof(struct __c1_dynamic_zone,disabled)!=428)return 1;if(__c1_zone_year(2006,&z,&a)||__c1_zone_year(2007,&z,&b))return 2;if(a.bias!=480||b.bias!=480||a.daylight.month!=4||a.daylight.day!=1||b.daylight.month!=3||b.daylight.day!=2)return 3;return 0;}
'@) 0
$a = [DateTimeOffset]::Parse('2006-03-20T12:00:00Z').ToUnixTimeSeconds()
$b = [DateTimeOffset]::Parse('2007-03-20T12:00:00Z').ToUnixTimeSeconds()
Check 'history-dst-change-roundtrip' ($prefix + $fixture + "int main(void){struct __c1_dynamic_zone z=pacific();struct tm v;if(__c1_dynamic_convert(${a}LL,&z,&v)||v.tm_year!=106||v.tm_hour!=4||v.tm_isdst)return 1;if(__c1_dynamic_mktime(&v,&z)!=${a}LL)return 2;if(__c1_dynamic_convert(${b}LL,&z,&v)||v.tm_year!=107||v.tm_hour!=5||v.tm_isdst!=1)return 3;if(__c1_dynamic_mktime(&v,&z)!=${b}LL)return 4;return 0;}") 0
Check 'history-disabled-fixed-and-range' ($prefix + $fixture + @'
int main(void){struct __c1_dynamic_zone z=pacific();struct __c1_zone r;struct tm v;z.disabled=1;z.base.bias=-840;if(__c1_dynamic_convert(1199102400LL,&z,&v)||v.tm_year!=108||v.tm_mon||v.tm_mday!=1||v.tm_hour!=2)return 1;if(__c1_zone_year(-10,&z,&r)||r.bias!=-840)return 2;z.disabled=0;errno=0;if(!__c1_zone_year(65536,&z,&r)||errno!=ERANGE)return 3;return 0;}
'@) 0
Check 'history-valid-minus-one-time' ($prefix + $fixture + @'
int main(void){struct __c1_dynamic_zone z=pacific();struct tm v;z.disabled=1;z.base.bias=0;if(__c1_dynamic_convert(-1,&z,&v))return 1;errno=EDOM;if(__c1_dynamic_mktime(&v,&z)!=-1||errno!=EDOM||v.tm_year!=69||v.tm_sec!=59)return 2;return 0;}
'@) 0
$rows = [Collections.Generic.List[string]]::new()
foreach ($date in '1991-06-01T12:00:00Z', '2006-03-20T12:00:00Z', '2007-03-20T12:00:00Z', '2024-01-01T00:30:00Z', '2024-12-31T23:30:00Z', '2030-06-01T12:00:00Z') {
    $utc = [DateTimeOffset]::Parse($date)
    $local = [TimeZoneInfo]::ConvertTime($utc, [TimeZoneInfo]::Local)
    $dst = [int][TimeZoneInfo]::Local.IsDaylightSavingTime($utc)
    $rows.Add('{' + $utc.ToUnixTimeSeconds() + 'LL,' + ($local.Year - 1900) + ',' + ($local.Month - 1) + ',' + $local.Day + ',' + $local.Hour + ',' + $local.Minute + ',' + $dst + '}')
}
$source = $prefix + 'static struct R{time_t t;int y,m,d,h,n,dst;} rows[]={' + ($rows -join ',') + "};`n" + @'
int main(void){struct tm *v,copy;int i;for(i=0;i<6;i++){v=localtime(&rows[i].t);if(!v||v->tm_year!=rows[i].y||v->tm_mon!=rows[i].m||v->tm_mday!=rows[i].d||v->tm_hour!=rows[i].h||v->tm_min!=rows[i].n||v->tm_isdst!=rows[i].dst)return 1;copy=*v;if(mktime(&copy)!=rows[i].t)return 2;}return 0;}
'@
Check 'history-host-local-independent-oracle' $source 0
