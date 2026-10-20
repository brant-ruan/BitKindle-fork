$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/clock.c`"`n"
    Check 'clock-type-units-monotonic' ($prefix + @'
#if CLOCKS_PER_SEC != 10000000
#error clock unit mismatch
#endif
int main(void){clock_t before=clock(),after;int i;if(sizeof(clock_t)!=8||before<0)return 1;for(i=0;i<50;i++){after=clock();if(after<before)return 2;before=after;}return 0;}
'@) 0
    Check 'clock-consumed-cpu' ($prefix + @'
int main(void){clock_t before=clock(),after;volatile unsigned int state=1;int i,batch;if(before<0)return 1;after=before;for(batch=0;batch<100&&after==before;batch++){for(i=0;i<5000000;i++)state=state*1664525U+1013904223U;after=clock();if(after<before)return 2;}if(after<=before)return 3;return 0;}
'@) 0
    Check 'clock-system-bracket' ($prefix + @'
int main(void){typedef int Times(void *,unsigned long long *,unsigned long long *,unsigned long long *,unsigned long long *);Times *times=*(Times **)0x401120;unsigned long long c,e,k1,u1,k2,u2;clock_t value;if(!times((void *)-1,&c,&e,&k1,&u1))return 1;value=clock();if(!times((void *)-1,&c,&e,&k2,&u2))return 2;if(value<0||(unsigned long long)value<k1+u1||(unsigned long long)value>k2+u2)return 3;return 0;}
'@) 0
    Check 'clock-representation-overflow' ($prefix + @'
int main(void){if(__c1_clock_combine(0,0)!=0||__c1_clock_combine(7,11)!=18)return 1;if(__c1_clock_combine(9223372036854775800ULL,7)!=9223372036854775807LL)return 2;errno=0;if(__c1_clock_combine(9223372036854775800ULL,8)!=-1||errno!=ERANGE)return 3;if(__c1_clock_combine(0,18446744073709551615ULL)!=-1)return 4;return 0;}
'@) 0
}
finally {
    $env:C1_INCLUDE = $savedInclude
}
