. (Join-Path $PSScriptRoot 'test-stdio-format.ps1')
Check 'format-decimal-basic' ($prefix + @'
int main(void){char b[256];sprintf(b,"%f/%e/%g",1.25,1.25,1.25);
if(strcmp(b,"1.250000/1.250000e+00/1.25"))return 1;
sprintf(b,"%#.0f/%#.3g/%+010.2f",2.0,1.0,-2.5);
return strcmp(b,"2./1.00/-000002.50");}
'@)
Check 'format-decimal-ties-carry' ($prefix + @'
int main(void){char b[256];sprintf(b,"%.0f/%.0f/%.0f/%.1f",0.5,1.5,2.5,1.25);
if(strcmp(b,"0/2/2/1.2"))return 1;
sprintf(b,"%.2e/%.3g/%.3g/%.3g",999.75,999.75,0.000099975,0.00099975);
return strcmp(b,"1.00e+03/1e+03/0.0001/0.001");}
'@)
Check 'format-decimal-directed' ($prefix + @'
int main(void){char b[128];fesetround(FE_UPWARD);
sprintf(b,"%.1f/%.1f/%.0f/%.0f",1.25,-1.25,0.00001,-0.00001);
if(strcmp(b,"1.3/-1.2/1/-0"))return 1;
fesetround(FE_DOWNWARD);sprintf(b,"%.1f/%.1f/%.0f/%.0f",1.25,-1.25,0.00001,-0.00001);
if(strcmp(b,"1.2/-1.3/0/-1"))return 2;
fesetround(FE_TOWARDZERO);sprintf(b,"%.1f/%.1f",1.99,-1.99);
return strcmp(b,"1.9/-1.9");}
'@)
Check 'format-decimal-specials' ($prefix + @'
int main(void){char b[128];sprintf(b,"%+08f/%F/%g/%.1f",INFINITY,-INFINITY,NAN,-0.0);
if(strcmp(b,"    +inf/-INF/nan/-0.0"))return 1;
return sprintf(b,"%.2Lf/%.2lf",(long double)1.5,2.5)!=9||strcmp(b,"1.50/2.50");}
'@)
Check 'format-decimal-subnormal' ($prefix + @'
int main(void){char b[1200];double x=__fpbits(double,1);
if(sprintf(b,"%.17e",x)!=24||strcmp(b,"4.94065645841246544e-324"))return 1;
if(snprintf(b,1200,"%.1074f",x)!=1076||strncmp(b,"0.",2))return 2;
if(strlen(b)!=1076||strcmp(b+1070,"265625"))return 3;
if(sprintf(b,"%.17g",__fpbits(double,0x7fefffffffffffffULL))!=23||strcmp(b,"1.7976931348623157e+308"))return 4;
return 0;}
'@)
Check 'format-decimal-truncation' ($prefix + @'
int main(void){char b[5];int n;
if(snprintf(b,5,"%.10000f%n",1.25,&n)!=10002||n!=10002||strcmp(b,"1.25"))return 1;
if(snprintf(NULL,0,"%.10000e",1.25)!=10006)return 2;
errno=0;if(snprintf(b,5,"%.2147483647f",1.25)!=-1||errno!=ERANGE)return 3;return 0;}
'@)
