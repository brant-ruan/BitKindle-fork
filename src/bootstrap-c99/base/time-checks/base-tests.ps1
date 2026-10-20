$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/time.c`"`n"
    Check 'time-epoch-negative' ($prefix + @'
int main(void){time_t t=0;struct tm *v=gmtime(&t);if(!v||v->tm_year!=70||v->tm_mon||v->tm_mday!=1||v->tm_wday!=4||v->tm_yday||v->tm_hour||v->tm_min||v->tm_sec||v->tm_isdst)return 1;t=-1;v=gmtime(&t);if(!v||v->tm_year!=69||v->tm_mon!=11||v->tm_mday!=31||v->tm_hour!=23||v->tm_min!=59||v->tm_sec!=59||v->tm_wday!=3||v->tm_yday!=364)return 2;return 0;}
'@) 0
    Check 'time-gmtime-failure-storage' ($prefix + @'
int main(void){time_t t=0;struct tm *p=gmtime(&t);t=9223372036854775807LL;errno=0;if(gmtime(&t)||errno!=ERANGE||p->tm_year!=70)return 1;t=(-9223372036854775807LL-1);if(gmtime(&t))return 2;t=86400;if(gmtime(&t)!=p||p->tm_mday!=2)return 3;return 0;}
'@) 0
    Check 'time-difference-extremes' ($prefix + @'
int main(void){time_t low=(-9223372036854775807LL-1),high=9223372036854775807LL;if(difftime(high,high-1)!=1||difftime(low+1,low)!=1)return 1;if(difftime(high,low)!=18446744073709551616.0||difftime(low,high)!=-18446744073709551616.0)return 2;if(difftime(0,0)!=0)return 3;return 0;}
'@) 0
    Check 'time-asctime-reference' ($prefix + @'
int main(void){time_t t=0;struct tm *v=gmtime(&t);char *p=asctime(v);if(!p||strcmp(p,"Thu Jan  1 00:00:00 1970\n"))return 1;v->tm_year=-1899;if(strcmp(asctime(v),"Thu Jan  1 00:00:00 1\n"))return 2;v->tm_sec=60;v->tm_year=100;if(strcmp(asctime(v),"Thu Jan  1 00:00:60 2000\n"))return 3;return 0;}
'@) 0
    Check 'time-asctime-invalid-extension' ($prefix + @'
int main(void){time_t t=0;struct tm v=*gmtime(&t);v.tm_wday=7;if(asctime(&v)||errno!=ERANGE)return 1;v=*gmtime(&t);v.tm_year=8100;if(asctime(&v))return 2;return 0;}
'@) 0
    $before = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds() - 5
    $after = $before + 120
    Check 'time-system-store' ($prefix + "int main(void){time_t t,r=time(&t);if(r!=t||r<${before}LL||r>${after}LL)return 1;r=time(0);if(r<t||r-t>10)return 2;return 0;}") 0
    $rows = [Collections.Generic.List[string]]::new()
    $dates = @([datetime]'0001-01-01', [datetime]'1600-02-29', [datetime]'1700-03-01', [datetime]'1900-03-01', [datetime]'2000-02-29', [datetime]'2100-03-01', [datetime]'2400-02-29', [datetime]'9999-12-31')
    $rng = [Random]::new(9377)
    for ($i = 0; $i -lt 192; $i++) {
        $y = $rng.Next(1, 10000)
        $m = $rng.Next(1, 13)
        $d = $rng.Next(1, [DateTime]::DaysInMonth($y, $m) + 1)
        $dates += [datetime]::new($y, $m, $d, $rng.Next(24), $rng.Next(60), $rng.Next(60))
    }
    foreach ($d in $dates) {
        $stamp = [DateTimeOffset]::new([datetime]::SpecifyKind($d, [DateTimeKind]::Utc)).ToUnixTimeSeconds()
        $rows.Add("{${stamp}LL,$($d.Year-1900),$($d.Month-1),$($d.Day),$($d.Hour),$($d.Minute),$($d.Second),$([int]$d.DayOfWeek),$($d.DayOfYear-1)}")
    }
    $source = $prefix + 'static struct R{time_t t;int y,m,d,h,n,s,w,j;} rows[]={' + ($rows -join ',') + "};`n" + @'
int main(void){int i;struct tm *p;for(i=0;i<200;i++){p=gmtime(&rows[i].t);if(!p||p->tm_year!=rows[i].y||p->tm_mon!=rows[i].m||p->tm_mday!=rows[i].d||p->tm_hour!=rows[i].h||p->tm_min!=rows[i].n||p->tm_sec!=rows[i].s||p->tm_wday!=rows[i].w||p->tm_yday!=rows[i].j||p->tm_isdst)return 1;}return 0;}
'@
    Check 'time-calendar-independent-oracle-200' $source 0
    . (Join-Path $PSScriptRoot 'local-tests.ps1')
    . (Join-Path $PSScriptRoot 'format-tests.ps1')
}
finally {
    $env:C1_INCLUDE = $savedInclude
}
