$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
Check 'scan-integer-bases' ($prefix + @'
int main(void){int a,b,c;unsigned int d,e,f;void *p;
if(sscanf("-17 017 0x2a 42 77 ff 0x1234","%d %i %i %u %o %x %p",&a,&b,&c,&d,&e,&f,&p)!=7)return 1;
return a!=-17||b!=15||c!=42||d!=42||e!=63||f!=255||p!=(void *)0x1234;}
'@)
Check 'scan-integer-lengths' ($prefix + @'
int main(void){signed char a;short b;long c;long long d,e,f,g;unsigned long long u;unsigned char h;unsigned short k;
if(sscanf("-128 -32768 -2147483648 -9223372036854775808 18446744073709551615 255 65535 -9 -8 -7","%hhd %hd %ld %lld %llu %hhu %hu %jd %zd %td",&a,&b,&c,&d,&u,&h,&k,&e,&f,&g)!=10)return 1;
return a!=-128||b!=-32768||c!=(-2147483647L-1)||d!=(-9223372036854775807LL-1)||u!=18446744073709551615ULL||h!=255||k!=65535||e!=-9||f!=-8||g!=-7;}
'@)
Check 'scan-counts-suppression-width' ($prefix + @'
int main(void){int a,b,n=-1;short h=-1;char c;
if(sscanf("  12345:67 X","%2d%*3d:%d%hn %c%n",&a,&b,&h,&c,&n)!=3)return 1;
if(a!=12||b!=67||h!=10||c!='X'||n!=12)return 2;
if(sscanf("0x1z","%2i%n",&a,&n)!=0)return 3;
return 0;}
'@)
Check 'scan-strings-and-scansets' ($prefix + @'
int main(void){char a[8],b[8],c[8],d[8];
if(sscanf(" abcXYZ 12-],done","%3s%[A-Z] %[]0-9-],%[^!]",a,b,c,d)!=4)return 1;
if(strcmp(a,"abc")||strcmp(b,"XYZ")||strcmp(c,"12-]")||strcmp(d,"done"))return 2;
if(sscanf("xyz","%[abc]",a)!=0)return 3;
if(sscanf(" a","%2c",a)!=1||a[0]!=' '||a[1]!='a')return 4;return 0;}
'@)
Check 'scan-failure-counts' ($prefix + @'
int main(void){int n=7,x=9;char b[8];
if(sscanf("","%d",&x)!=EOF||x!=9)return 1;
if(sscanf("x","%d",&x)!=0||x!=9)return 2;
if(sscanf("","%n%d",&n,&x)!=0||n!=0)return 3;
if(sscanf("1","%*d%d",&x)!=0)return 4;
if(sscanf("+","%d",&x)!=0)return 5;
if(sscanf("ab","%3c",b)!=0)return 6;
if(sscanf("ab","%2147483647c",b)!=0)return 7;
if(sscanf(""," ")==EOF)return 8;return 0;}
'@)
Check 'scan-stream-lookahead' ($prefix + @'
int main(void){FILE *f=tmpfile();int n,x;char c;
if(!f||fputs("12x 0xZ",f)||fseek(f,0,SEEK_SET))return 1;
if(fscanf(f,"%d%n",&x,&n)!=1||x!=12||n!=2||ftell(f)!=2||fgetc(f)!='x')return 2;
if(fscanf(f," %i",&x)!=0||fgetc(f)!='Z')return 3;
if(fscanf(f,"%c",&c)!=EOF||!feof(f))return 4;return fclose(f);}
'@)
Check 'scan-va-copy' ($prefix + @'
int helper(const char *s,const char *fmt,...){va_list a,b;int n,m;va_start(a,fmt);va_copy(b,a);n=vsscanf(s,fmt,a);m=vsscanf(s,fmt,b);va_end(a);va_end(b);return n==m?n:-9;}
int main(void){int n;char s[8];if(helper("17 hello","%d %s",&n,s)!=2)return 1;return n!=17||strcmp(s,"hello");}
'@)
Check 'scan-wide-c-locale' ($prefix + @'
int main(void){wchar_t a[8],b[8],c[8];
if(sscanf("A\xff rest 12","%2lc %ls %l[0-9]",a,b,c)!=3)return 1;
return a[0]!=65||a[1]!=255||b[0]!='r'||b[4]||c[0]!='1'||c[1]!='2'||c[2];}
'@)
Check 'scan-standard-pipe' ($prefix + @'
int main(void){int a,b;char c;if(scanf("%d,%d%c",&a,&b,&c)!=3||a!=17||b!=29||c!='\n')return 1;
return scanf("%d",&a)!=EOF;}
'@) 0 $false "17,29`r`n"
