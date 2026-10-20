. (Join-Path $PSScriptRoot 'test-stdio-decimal.ps1')
Check 'format-hex-basic' ($prefix + @'
int main(void){char b[128];sprintf(b,"%a/%A/%#.0a",1.5,26.0,0.0);
return strcmp(b,"0x1.8p+0/0X1.AP+4/0x0.p+0");}
'@)
Check 'format-hex-rounding' ($prefix + @'
int main(void){char b[128];sprintf(b,"%.0a/%.0a/%.1a",1.5,1.25,1.96875);
if(strcmp(b,"0x1p+1/0x1p+0/0x1.0p+1"))return 1;
fesetround(FE_UPWARD);sprintf(b,"%.0a/%.0a",1.25,-1.25);
if(strcmp(b,"0x1p+1/-0x1p+0"))return 2;
fesetround(FE_DOWNWARD);sprintf(b,"%.0a/%.0a",1.25,-1.25);
if(strcmp(b,"0x1p+0/-0x1p+1"))return 3;
fesetround(FE_TOWARDZERO);sprintf(b,"%.0a",1.75);return strcmp(b,"0x1p+0");}
'@)
Check 'format-hex-range' ($prefix + @'
int main(void){char b[128];sprintf(b,"%a/%a",__fpbits(double,1),__fpbits(double,0x7fefffffffffffffULL));
if(strcmp(b,"0x1p-1074/0x1.fffffffffffffp+1023"))return 1;
sprintf(b,"%.0a",__fpbits(double,0x7fefffffffffffffULL));return strcmp(b,"0x1p+1024");}
'@)
Check 'format-hex-padding-counts' ($prefix + @'
int main(void){char b[128];sprintf(b,"%+015.1a/%a",1.5,-0.0);
if(strcmp(b,"+0x0000001.8p+0/-0x0p+0"))return 1;
if(snprintf(b,8,"%.10000a",1.5)!=10007||strcmp(b,"0x1.800"))return 2;
if(sprintf(b,"%010A",INFINITY)!=10||strcmp(b,"       INF"))return 3;return 0;}
'@)
Check 'format-hex-exact-roundtrip' ($prefix + @'
int main(void){unsigned long long bits=1;int i;union{double f;unsigned long long u;}a,b;char text[80],*end;
for(i=0;i<1024;i++){
bits=bits*6364136223846793005ULL+1;a.u=bits;
if(((bits>>52)&2047)==2047)continue;
if(sprintf(text,"%a",a.f)<0)return 1;b.f=strtod(text,&end);
if(*end||b.u!=bits)return 2;}
return 0;}
'@)
