$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
Check 'format-integer-limits' ($prefix + @'
int main(void){char b[256];int n=sprintf(b,"%d %u %ld %lu %lld %llu",(-2147483647-1),4294967295U,(-2147483647L-1),4294967295UL,(-9223372036854775807LL-1),18446744073709551615ULL);
return n!=(int)strlen(b)||strcmp(b,"-2147483648 4294967295 -2147483648 4294967295 -9223372036854775808 18446744073709551615");}
'@)
Check 'format-flags-and-precision' ($prefix + @'
int main(void){char b[256];sprintf(b,"[%+08d][%#08x][%-6.4d][%#.0o][%.0x][% .0d]",-12,42,23,0,0,0);
if(strcmp(b,"[-0000012][0x00002a][0023  ][0][][ ]"))return 1;
sprintf(b,"[%#6.4o][%#6.4x][%-06d][%+ d]",9,10,3,4);
return strcmp(b,"[  0011][0x000a][3     ][+4]");}
'@)
Check 'format-star-and-strings' ($prefix + @'
int main(void){char b[128];int n=sprintf(b,"[%*.*s][%0*.*d][%c]%%",-6,3,"abcdef",7,4,12,65);
if(strcmp(b,"[abc   ][   0012][A]%")||n!=(int)strlen(b))return 1;
sprintf(b,"%.*s/%.*d",-1,"all",-1,3);return strcmp(b,"all/3");}
'@)
Check 'format-short-lengths' ($prefix + @'
int main(void){char b[128];sprintf(b,"%hhd %hhu %hd %hu %jd %zu %td",130,258,65535,65537,(long long)-9,(size_t)123,(ptrdiff_t)-4);
return strcmp(b,"-126 2 -1 1 -9 123 -4");}
'@)
Check 'format-n-and-truncation' ($prefix + @'
int main(void){char b[8];int n=-1;short h=-1;signed char hh=-1;long l=-1;long long ll=-1,z=-1;
memset(b,'Q',8);if(snprintf(b,4,"ab%nC%hnD%hhnE%lnF%llnG%znH",&n,&h,&hh,&l,&ll,&z)!=8)return 1;
if(strcmp(b,"abC")||b[4]!='Q'||n!=2||h!=3||hh!=4||l!=5||ll!=6||z!=7)return 2;
if(snprintf(NULL,0,"%010d/%s",12,"end")!=14)return 3;
if(snprintf(b,1,"long")!=4||b[0])return 4;return 0;}
'@)
Check 'format-large-logical-width' ($prefix + @'
int main(void){char b[4];int n=-1;
if(snprintf(b,4,"%2147483647d%n",1,&n)!=2147483647||n!=2147483647||strcmp(b,"   "))return 1;
errno=0;if(snprintf(b,4,"%2147483647dX",1)!=-1||errno!=ERANGE)return 2;
errno=0;if(snprintf(b,4,"%2147483648d",1)!=-1||errno!=ERANGE)return 3;return 0;}
'@)
Check 'format-va-copy' ($prefix + @'
int helper(char *a,char *b,const char *f,...){va_list x,y;int n,m;
va_start(x,f);va_copy(y,x);n=vsprintf(a,f,x);m=vsnprintf(b,128,f,y);va_end(y);va_end(x);return n!=m;}
int main(void){char a[128],b[128];if(helper(a,b,"%d/%s/%llx/%c",7,"text",0xfedcba9876543210ULL,'Q'))return 1;
return strcmp(a,b)||strcmp(a,"7/text/fedcba9876543210/Q");}
'@)
Check 'format-standard-stream' ($prefix + @'
int helper(const char *f,...){va_list ap;int n;va_start(ap,f);n=vprintf(f,ap);va_end(ap);return n;}
int main(void){if(printf("%s %04d\n","line",7)!=10||helper("%#X",255)!=4)return 1;return 0;}
'@) 0 $false '' "line 0007`r`n0XFF"
Check 'format-file-error' ($prefix + @'
int main(void){FILE *f=tmpfile();char b[32];void *saved;
if(!f||fprintf(f,"%d:%s",7,"text")!=6||fseek(f,0,SEEK_SET)||fread(b,1,32,f)!=6||memcmp(b,"7:text",6))return 1;
if(fclose(f))return 2;f=tmpfile();if(!f||setvbuf(f,NULL,_IONBF,0))return 3;
saved=f->handle;f->handle=(void *)-1;
if(fprintf(f,"write")!=-1||!ferror(f))return 4;f->handle=saved;return fclose(f);}
'@)
