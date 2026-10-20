. (Join-Path $PSScriptRoot 'test-stdio-base.ps1')
Check 'stdio-full-buffer-visibility' ($prefix + @'
int main(void){FILE *f,*r;char store[9];
f=fopen(PATH,"wb");if(!f||setvbuf(f,store,_IOFBF,8))return 1;
store[8]='Z';if(fputs("abc",f)||ftell(f)!=3)return 2;
r=fopen(PATH,"rb");if(!r||fgetc(r)!=EOF)return 3;fclose(r);
if(fflush(f)||store[8]!='Z')return 4;
r=fopen(PATH,"rb");if(!r||fgetc(r)!='a')return 5;fclose(r);
if(fwrite("defghij",1,7,f)!=7||fclose(f))return 6;
r=fopen(PATH,"rb");if(!r||fseek(r,0,SEEK_END)||ftell(r)!=10)return 7;
return fclose(r)||remove(PATH);}
'@)
Check 'stdio-line-buffer' ($prefix + @'
int main(void){FILE *f,*r;char b[16],store[3];
f=fopen(PATH,"w");if(!f||setvbuf(f,store,_IOLBF,3))return 1;
if(fputc('A',f)!='A')return 2;
r=fopen(PATH,"rb");if(!r||fgetc(r)!=EOF)return 3;fclose(r);
if(fputc('\n',f)!='\n'||fputc('B',f)!='B'||ftell(f)!=4)return 4;
r=fopen(PATH,"rb");if(!r||fread(b,1,16,r)!=3||memcmp(b,"A\r\n",3))return 5;fclose(r);
if(fclose(f))return 6;
r=fopen(PATH,"rb");if(!r||fread(b,1,16,r)!=4||memcmp(b,"A\r\nB",4))return 7;
return fclose(r)||remove(PATH);}
'@)
Check 'stdio-buffered-read-position' ($prefix + @'
int main(void){FILE *f;char b[16];fpos_t p;
f=fopen(PATH,"wb");if(!f||fputs("A\rB\r\nC",f)||fclose(f))return 1;
f=fopen(PATH,"r");if(!f||setvbuf(f,NULL,_IOFBF,4))return 2;
if(fgetc(f)!='A'||fgetc(f)!='\r'||ftell(f)!=2||fgetpos(f,&p))return 3;
if(fgetc(f)!='B'||fgetc(f)!='\n'||fsetpos(f,&p)||fgetc(f)!='B')return 4;
if(fseek(f,0,SEEK_CUR)||ftell(f)!=3||fgetc(f)!='\n'||fgetc(f)!='C')return 5;
if(fgetc(f)!=EOF||!feof(f)||ferror(f))return 6;
rewind(f);if(fread(b,1,16,f)!=5||memcmp(b,"A\rB\nC",5))return 7;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-buffered-update' ($prefix + @'
int main(void){FILE *f;char b[8];
f=fopen(PATH,"w+b");if(!f||setvbuf(f,NULL,_IOFBF,7))return 1;
if(fwrite("abcdef",1,6,f)!=6||fseek(f,0,SEEK_SET))return 2;
if(fgetc(f)!='a'||fseek(f,0,SEEK_CUR)||fwrite("XY",1,2,f)!=2||ftell(f)!=3)return 3;
if(fseek(f,0,SEEK_SET)||fread(b,1,6,f)!=6||memcmp(b,"aXYdef",6))return 4;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-buffered-append' ($prefix + @'
int main(void){FILE *f;char b[8];
f=fopen(PATH,"wb");if(!f||fputs("abc",f)||fclose(f))return 1;
f=fopen(PATH,"a+b");if(!f||setvbuf(f,NULL,_IOFBF,8))return 2;
if(fseek(f,0,SEEK_SET)||fputc('X',f)!='X'||ftell(f)!=4||fflush(f))return 3;
if(fseek(f,0,SEEK_SET)||fread(b,1,4,f)!=4||memcmp(b,"abcX",4))return 4;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-buffer-drain-error-retry' ($prefix + @'
int main(void){FILE *f;void *saved;char b[8];
f=fopen(PATH,"w+b");if(!f||setvbuf(f,NULL,_IOFBF,8)||fputs("abc",f))return 1;
saved=f->handle;f->handle=(void *)-1;
if(fflush(f)!=EOF||!ferror(f))return 2;
f->handle=saved;clearerr(f);if(fflush(f)||fseek(f,0,SEEK_SET))return 3;
if(fread(b,1,8,f)!=3||memcmp(b,"abc",3))return 4;
return fclose(f)||remove(PATH);}
'@)
Check 'stdio-flush-all-buffers' ($prefix + @'
int main(void){FILE *a,*b,*r;
a=fopen(PATH,"wb");b=fopen(OTHER,"wb");if(!a||!b)return 1;
if(setvbuf(a,NULL,_IOFBF,8)||setvbuf(b,NULL,_IOFBF,8)||fputs("A",a)||fputs("B",b)||fflush(NULL))return 2;
r=fopen(PATH,"rb");if(!r||fgetc(r)!='A'||fclose(r))return 3;
r=fopen(OTHER,"rb");if(!r||fgetc(r)!='B'||fclose(r))return 4;
return fclose(a)||fclose(b)||remove(PATH)||remove(OTHER);}
'@)
Check 'stdio-setbuf-and-reopen' ($prefix + @'
int main(void){FILE *f;char buffer[BUFSIZ];
f=fopen(PATH,"wb");if(!f)return 1;
if(setvbuf(f,NULL,9,8)==0||setvbuf(f,NULL,_IOFBF,0)==0)return 2;
setbuf(f,buffer);if(fputs("saved",f))return 3;
if(freopen(PATH,"rb",f)!=f||fgetc(f)!='s')return 4;
if(fclose(f))return 5;
f=fopen(PATH,"wb");if(!f)return 6;setbuf(f,NULL);
if(fputs("x",f)||fclose(f))return 7;return remove(PATH);}
'@)
Check 'stdio-buffered-pipe' ($prefix + @'
int main(void){char b[16];if(setvbuf(stdin,NULL,_IOFBF,3)||setvbuf(stdout,NULL,_IOLBF,2))return 1;
if(fread(b,1,16,stdin)!=5||memcmp(b,"abc\nd",5)||!feof(stdin)||ferror(stdin))return 2;
if(puts("OK")||fclose(stdout))return 3;return 0;}
'@) 0 $false "abc`r`nd" "OK`r`n"
