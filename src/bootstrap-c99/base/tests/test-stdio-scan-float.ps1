. (Join-Path $PSScriptRoot 'test-stdio-scan.ps1')
Check 'scan-floating-types-and-syntax' ($prefix + @'
int main(void){float a;double b,c;long double d;int n;
if(sscanf("1.25 -2.5e2 0x1.8p+2 .125","%f %le %la %Lg%n",&a,&b,&c,&d,&n)!=4)return 1;
return a!=1.25f||b!=-250.0||c!=6.0||d!=0.125L||n!=25;}
'@)
Check 'scan-floating-incomplete-prefix' ($prefix + @'
int main(void){FILE *f=tmpfile();double x=7.0;char c;
if(!f||fputs("100er 0xZ +!",f)||fseek(f,0,SEEK_SET))return 1;
if(fscanf(f,"%lf",&x)!=0||x!=7.0||fgetc(f)!='r')return 2;
if(fscanf(f," %lf",&x)!=0||fgetc(f)!='Z')return 3;
if(fscanf(f," %lf",&x)!=0||fgetc(f)!='!')return 4;return fclose(f);}
'@)
Check 'scan-floating-width' ($prefix + @'
int main(void){double x;char c;int n=99;
if(sscanf("12e3","%2lf%c%n",&x,&c,&n)!=2||x!=12.0||c!='e'||n!=3)return 1;
if(sscanf("12e3","%3lf%n",&x,&n)!=0||n!=3)return 2;
if(sscanf("0x1p2","%3lf%c",&x,&c)!=2||x!=1.0||c!='p')return 3;
if(sscanf("infinity","%3lf%c",&x,&c)!=2||!isinf(x)||c!='i')return 4;
if(sscanf("infinity","%4lf",&x)!=0)return 5;return 0;}
'@)
Check 'scan-floating-specials' ($prefix + @'
int main(void){double a,b,c,d;char next;
if(sscanf("-INF +Infinity NaN(tag_12) -nan()!","%lf %lf %lf %lf%c",&a,&b,&c,&d,&next)!=5)return 1;
return !isinf(a)||!signbit(a)||!isinf(b)||signbit(b)||!isnan(c)||!isnan(d)||!signbit(d)||next!='!';}
'@)
Check 'scan-floating-nan-prefix' ($prefix + @'
int main(void){FILE *f=tmpfile();double x=9.0;
if(!f||fputs("nan(tag!",f)||fseek(f,0,SEEK_SET))return 1;
if(fscanf(f,"%lf",&x)!=0||x!=9.0||fgetc(f)!='!')return 2;return fclose(f);}
'@)
Check 'scan-float-no-double-rounding' ($prefix + @'
int main(void){union{float f;unsigned int u;}a;
if(sscanf("1.0000000596046448","%f",&a.f)!=1||a.u!=0x3f800001)return 1;
fesetround(FE_DOWNWARD);if(sscanf("1.0000000596046448","%f",&a.f)!=1||a.u!=0x3f800000)return 2;return 0;}
'@)
Check 'scan-floating-long-token' ($prefix + @'
int main(void){char text[5100];double x;int n;memset(text,'0',5001);text[0]='0';text[1]='.';text[5001]='1';strcpy(text+5002,"e5000!");
if(sscanf(text,"%lf%n",&x,&n)!=1||x!=1.0||n!=5007)return 1;return 0;}
'@)
Check 'scan-floating-suppressed' ($prefix + @'
int main(void){int x;double y;
if(sscanf("1.2e3 17","%*lf %d",&x)!=1||x!=17)return 1;
if(sscanf("1.2e3","%*f %lf",&y)!=0)return 2;
if(sscanf("","%lf",&y)!=EOF||sscanf(".","%lf",&y)!=0)return 3;return 0;}
'@)
