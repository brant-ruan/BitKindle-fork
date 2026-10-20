$prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/localtime.c`"`n"
$north = @'
static struct __c1_zone north(void){struct __c1_zone z;memset(&z,0,sizeof(z));z.bias=300;z.daylight_bias=-60;z.daylight.month=3;z.daylight.day=2;z.daylight.hour=2;z.standard.month=11;z.standard.day=1;z.standard.hour=2;return z;}
'@+ "`n"
Check 'local-zone-layout-fixed-offset' ($prefix + @'
int main(void){struct __c1_zone z;struct tm v;memset(&z,0,sizeof(z));if(sizeof(z)!=172)return 1;z.bias=-330;z.standard_bias=999;if(__c1_zone_convert(0,&z,&v)||v.tm_hour!=5||v.tm_min!=30||v.tm_mday!=1||v.tm_isdst)return 2;return 0;}
'@) 0
Check 'local-northern-transition-boundaries' ($prefix + $north + @'
int main(void){struct __c1_zone z=north();struct tm v;if(__c1_zone_convert(1710053999LL,&z,&v)||v.tm_hour!=1||v.tm_min!=59||v.tm_sec!=59||v.tm_isdst)return 1;if(__c1_zone_convert(1710054000LL,&z,&v)||v.tm_hour!=3||v.tm_min||v.tm_isdst!=1)return 2;if(__c1_zone_convert(1730613599LL,&z,&v)||v.tm_hour!=1||v.tm_min!=59||v.tm_isdst!=1)return 3;if(__c1_zone_convert(1730613600LL,&z,&v)||v.tm_hour!=1||v.tm_min||v.tm_isdst)return 4;return 0;}
'@) 0
Check 'local-southern-rules' ($prefix + @'
int main(void){struct __c1_zone z;struct tm v;memset(&z,0,sizeof(z));z.bias=-600;z.daylight_bias=-60;z.daylight.month=10;z.daylight.day=1;z.daylight.hour=2;z.standard.month=4;z.standard.day=1;z.standard.hour=3;if(__c1_zone_convert(1704067200LL,&z,&v)||v.tm_hour!=11||v.tm_isdst!=1)return 1;if(__c1_zone_convert(1719792000LL,&z,&v)||v.tm_hour!=10||v.tm_isdst)return 2;return 0;}
'@) 0
Check 'mktime-fold-choices' ($prefix + $north + @'
int main(void){struct __c1_zone z=north();struct tm v;time_t r;memset(&v,0,sizeof(v));v.tm_year=124;v.tm_mon=10;v.tm_mday=3;v.tm_hour=1;v.tm_min=30;v.tm_isdst=-1;r=__c1_zone_mktime(&v,&z);if(r!=1730615400LL||v.tm_isdst)return 1;v.tm_isdst=1;r=__c1_zone_mktime(&v,&z);if(r!=1730611800LL||v.tm_isdst!=1)return 2;return 0;}
'@) 0
Check 'mktime-gap-forward' ($prefix + $north + @'
int main(void){struct __c1_zone z=north();struct tm v;memset(&v,0,sizeof(v));v.tm_year=124;v.tm_mon=2;v.tm_mday=10;v.tm_hour=2;v.tm_min=30;v.tm_isdst=-1;if(__c1_zone_mktime(&v,&z)!=1710055800LL||v.tm_hour!=3||v.tm_min!=30||v.tm_isdst!=1)return 1;return 0;}
'@) 0
Check 'mktime-normalize-preserve-gmtime' ($prefix + @'
int main(void){struct __c1_zone z;struct tm v,*saved;time_t t=0;memset(&z,0,sizeof(z));memset(&v,0,sizeof(v));saved=gmtime(&t);v.tm_year=124;v.tm_mon=14;v.tm_mday=0;v.tm_hour=25;v.tm_min=-1;v.tm_sec=61;v.tm_isdst=-1;t=__c1_zone_mktime(&v,&z);if(t!=1740790801LL||v.tm_year!=125||v.tm_mon!=2||v.tm_mday!=1||v.tm_hour!=1||v.tm_min||v.tm_sec!=1)return 1;if(saved->tm_year!=70||saved->tm_mday!=1)return 2;return 0;}
'@) 0
Check 'local-public-roundtrip-ctime' ($prefix + @'
int main(void){time_t t=time(0),r;struct tm v;char expected[26],*p=ctime(&t);if(!p)return 1;strcpy(expected,p);v=*localtime(&t);r=mktime(&v);if(r!=t||strcmp(expected,asctime(&v)))return 2;return 0;}
'@) 0
Check 'local-overflow' ($prefix + @'
int main(void){struct __c1_zone z;struct tm v;memset(&z,0,sizeof(z));z.bias=-60;errno=0;if(!__c1_zone_convert(9223372036854775807LL,&z,&v)||errno!=ERANGE)return 1;return 0;}
'@) 0
