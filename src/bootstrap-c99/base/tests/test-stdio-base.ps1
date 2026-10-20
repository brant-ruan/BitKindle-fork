$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
$dir = $ScratchDirectory.Replace('\', '/')
$prefix += ('#define PATH "' + $dir + '/stream.bin"') + [char]10
$prefix += ('#define OTHER "' + $dir + '/renamed.bin"') + [char]10
Check 'stdio-binary-bytes' ($prefix + @'
int main(void){FILE *f;unsigned char a[256],b[260];int i;
for(i=0;i<256;i++)a[i]=i;
f=fopen(PATH,"wb");if(!f)return 1;
if(fwrite(a,16,16,f)!=16||fflush(f)||fclose(f))return 2;
f=fopen(PATH,"rb");if(!f)return 3;
if(fread(b,3,86,f)!=85||!feof(f)||ferror(f))return 4;
for(i=0;i<256;i++)if(b[i]!=i)return 5;
if(ungetc(255,f)!=255||feof(f)||fgetc(f)!=255||fgetc(f)!=EOF)return 6;
clearerr(f);if(feof(f)||ferror(f))return 7;
rewind(f);if(ftell(f)||fread(b,1,256,f)!=256||feof(f))return 8;
if(fgetc(f)!=EOF||!feof(f))return 9;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-text-translation' ($prefix + @'
int main(void){FILE *f;char b[32];fpos_t p;
f=fopen(PATH,"w");if(!f)return 1;
if(fputs("A\nB\rC\r\nD\r",f)==EOF||fclose(f))return 2;
f=fopen(PATH,"rb");if(!f)return 3;
if(fread(b,1,32,f)!=11||memcmp(b,"A\r\nB\rC\r\r\nD\r",11))return 4;
fclose(f);f=fopen(PATH,"r");if(!f)return 5;
if(fgetc(f)!='A'||fgetc(f)!='\n'||ftell(f)!=3)return 6;
if(fgetc(f)!='B'||fgetc(f)!='\r'||ftell(f)!=5||fgetpos(f,&p))return 7;
if(fgetc(f)!='C'||fsetpos(f,&p)||fgetc(f)!='C')return 8;
if(fgetc(f)!='\r'||fseek(f,0,SEEK_CUR)||fgetc(f)!='\n')return 9;
if(fgetc(f)!='D'||fgetc(f)!='\r'||feof(f)||fgetc(f)!=EOF||!feof(f))return 10;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-append-reopen' ($prefix + @'
int main(void){FILE *f,*g;char b[8];
f=fopen(PATH,"wb");if(!f)return 1;fputs("abc",f);fclose(f);
f=fopen(PATH,"a+b");if(!f||fgetc(f)!='a'||fseek(f,0,SEEK_SET))return 2;
if(fputc('X',f)!='X'||fseek(f,0,SEEK_SET)||fread(b,1,4,f)!=4||memcmp(b,"abcX",4))return 3;
g=freopen(PATH,"w+b",f);if(g!=f||ftell(f)||fgetc(f)!=EOF)return 4;
if(fseek(f,0,SEEK_SET)||fputc('Z',f)!='Z'||fclose(f))return 5;
if(rename(PATH,OTHER))return 6;
f=fopen(OTHER,"rb");if(!f||fgetc(f)!='Z'||fgetc(f)!=EOF)return 7;
return fclose(f)||remove(OTHER);}
'@)
Check 'stdio-errors-lines' ($prefix + @'
int main(void){FILE *f;char b[8];
if(fopen(PATH,"q")||errno!=22||fopen(PATH,"r++"))return 1;
f=fopen(PATH,"w+");if(!f)return 2;
if(fputs("ab\nc",f)||fseek(f,0,SEEK_SET))return 3;
if(fgets(b,1,f)!=b||b[0]||ftell(f))return 4;
if(!fgets(b,8,f)||strcmp(b,"ab\n")||!fgets(b,8,f)||strcmp(b,"c"))return 5;
if(!feof(f)||fgets(b,8,f))return 6;
fclose(f);f=fopen(PATH,"r");if(!f)return 7;
if(fputc('x',f)!=EOF||!ferror(f))return 8;
clearerr(f);if(ferror(f)||fgetc(f)!='a')return 9;
if(fread(b,0,100,f)||fwrite(b,0,100,f)||ferror(f))return 10;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-large-transfer' ($prefix + @'
int main(void){size_t n=2097189,i;unsigned char *a=malloc(n),*b=malloc(n);FILE *f;
if(!a||!b)return 1;for(i=0;i<n;i++)a[i]=(i*37+i/251)&255;
f=fopen(PATH,"w+b");if(!f)return 2;
if(fwrite(a,1,n,f)!=n||ftell(f)!=(long)n||fseek(f,0,SEEK_SET))return 3;
if(fread(b,1,n,f)!=n||memcmp(a,b,n)||feof(f))return 4;
free(a);free(b);return fclose(f)||remove(PATH);}
'@)
Check 'stdio-standard-streams' ($prefix + @'
int main(void){char b[16];if(getchar()!='A'||!fgets(b,16,stdin)||strcmp(b,"B\n"))return 1;
if(puts("line")==EOF||putchar('X')!='X'||fflush(NULL))return 2;return 0;}
'@) 0 $false "AB`r`n" "line`r`nX"
Check 'stdio-sticky-error-success' ($prefix + @'
int main(void){FILE *f;char b[8];
f=fopen(PATH,"wb");if(!f||fputs("tail",f)||fclose(f))return 1;
f=fopen(PATH,"r");if(!f||fputc('x',f)!=EOF||!ferror(f))return 2;
if(fgets(b,8,f)!=b||strcmp(b,"tail")||!feof(f)||!ferror(f))return 3;
rewind(f);if(feof(f)||ferror(f)||ftell(f)||fgetc(f)!='t')return 4;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-open-capacity' ($prefix + @'
int main(void){FILE *f[FOPEN_MAX],*g;int i,n=FOPEN_MAX-3;
g=fopen(PATH,"wb");if(!g||fclose(g))return 1;
for(i=0;i<n;i++){f[i]=fopen(PATH,"rb");if(!f[i])return 2;}
if(fopen(PATH,"rb")||errno!=24)return 3;
if(fclose(f[n/2]))return 4;g=fopen(PATH,"rb");if(!g)return 5;
for(i=0;i<n;i++)if(i!=n/2&&fclose(f[i]))return 6;
return fclose(g)||remove(PATH);}
'@)
Check 'stdio-position-and-push' ($prefix + @'
int main(void){FILE *f;char b[8];fpos_t pos;
f=fopen(PATH,"w+b");if(!f||fwrite("012345",2,3,f)!=3||fseek(f,-3,SEEK_END))return 1;
if(ftell(f)!=3||fgetc(f)!='3'||ungetc('X',f)!='X'||ungetc('Y',f)!=EOF||ftell(f)!=3)return 2;
if(fread(b,1,3,f)!=3||memcmp(b,"X45",3))return 3;
if(fgetpos(f,&pos)||fseek(f,0,SEEK_SET)||fsetpos(f,&pos)||ftell(f)!=6)return 4;
if(fgetc(f)!=EOF||!feof(f)||fseek(f,0,SEEK_SET)||feof(f))return 5;
if(ungetc('Q',f)!='Q'||fseek(f,0,SEEK_SET)||fgetc(f)!='0')return 6;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-raw-io-errors' ($prefix + @'
int main(void){FILE *f;void *saved;char b[4];
f=fopen(PATH,"w+b");if(!f)return 1;saved=f->handle;f->handle=(void *)-1;
if(fwrite("abc",1,3,f)||!ferror(f)||errno!=9)return 2;
clearerr(f);if(fread(b,1,3,f)||!ferror(f)||feof(f)||errno!=9)return 3;
clearerr(f);if(fseek(f,0,SEEK_SET)==0||!ferror(f))return 4;
f->handle=saved;clearerr(f);if(fputs("ok",f)||fseek(f,0,SEEK_SET)||!fgets(b,4,f)||strcmp(b,"ok"))return 5;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-wide-file-position' ($prefix + @'
int main(void){FILE *f;fpos_t p,q;f=fopen(PATH,"w+b");if(!f)return 1;
p.offset=4294967313LL;if(fsetpos(f,&p)||fgetpos(f,&q)||q.offset!=p.offset)return 2;
errno=0;if(ftell(f)!=-1||errno!=ERANGE)return 3;
/* Seeking alone does not extend the file or allocate a huge test fixture. */
if(fseek(f,0,SEEK_END)||ftell(f))return 4;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-gets' ($prefix + @'
int main(void){char b[16];if(gets(b)!=b||strcmp(b,"abc")||gets(b)!=b||b[0])return 1;
if(gets(b)!=b||strcmp(b,"tail")||!feof(stdin))return 2;
b[0]='X';if(gets(b)||b[0]!='X')return 3;return 0;}
'@) 0 $false "abc`r`n`r`ntail"
Check 'stdio-perror' ($prefix + @'
int main(void){FILE *f;char b[256],want[256];
if(freopen(PATH,"w",stderr)!=stderr)return 1;
errno=EDOM;strcpy(want,"probe: ");strcat(want,strerror(EDOM));strcat(want,"\n");
perror("probe");if(errno!=EDOM)return 2;
perror("");perror(NULL);if(fclose(stderr))return 3;
f=fopen(PATH,"r");if(!f||!fgets(b,256,f)||strcmp(b,want))return 4;
strcpy(want,strerror(EDOM));strcat(want,"\n");
if(!fgets(b,256,f)||strcmp(b,want)||!fgets(b,256,f)||strcmp(b,want)||fgetc(f)!=EOF)return 5;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-pipe-eof-text' ($prefix + @'
int main(void){char b[16];if(fread(b,1,16,stdin)!=5||memcmp(b,"tail\r",5))return 1;
if(!feof(stdin)||ferror(stdin)||fgetc(stdin)!=EOF)return 2;
clearerr(stdin);if(fgetc(stdin)!=EOF||!feof(stdin)||ferror(stdin))return 3;return 0;}
'@) 0 $false "tail`r"
Check 'stdio-pipe-eof-binary' ($prefix + @'
int main(void){char b[16];stdin->binary=1;
if(fread(b,3,4,stdin)!=1||memcmp(b,"abcde",5)||!feof(stdin)||ferror(stdin))return 1;
if(ungetc('X',stdin)!='X'||fgetc(stdin)!='X'||fgetc(stdin)!=EOF||ferror(stdin))return 2;
return 0;}
'@) 0 $false 'abcde'
