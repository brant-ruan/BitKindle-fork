# Run the whole stream suite against default-buffered implementation.
$body = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'test-stdio-base.ps1'))
$body = $body.Replace('src/bootstrap-c99/base/lib/runtime.c', 'src/bootstrap-c99/base/lib/runtime.c')
$body = $body.Replace('if(!f)return 1;saved=f->handle;', 'if(!f)return 1;setbuf(f,NULL);saved=f->handle;')
Invoke-Expression $body
$buffer = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'test-stdio-buffer.ps1'))
$buffer = $buffer.Substring($buffer.IndexOf("Check 'stdio-full-buffer-visibility'"))
Invoke-Expression $buffer
Check 'stdio-default-main-flush' ($prefix + @'
int main(void){if(fputs("buffered",stdout))return 1;return 71;}
'@) 71 $false '' 'buffered'
Check 'stdio-default-exit-flush' ($prefix + @'
int main(void){fputs("buffered",stdout);exit(72);return 1;}
'@) 72 $false '' 'buffered'
Check 'stdio-default-immediate-no-flush' ($prefix + @'
int main(void){fputs("buffered",stdout);_Exit(73);return 1;}
'@) 73
Check 'stdio-default-handler-before-flush' ($prefix + @'
void a(void){fputs("A",stdout);}void b(void){fputs("B",stdout);}
int main(void){atexit(a);fputs("first",stdout);atexit(b);return 0;}
'@) 0 $false '' 'firstBA'
Check 'stdio-default-late-init-handler' ($prefix + @'
void a(void){fputs("late",stdout);}
int main(void){atexit(a);return 0;}
'@) 0 $false '' 'late'
Check 'stdio-default-prompt-flush' ($prefix + @'
int main(void){FILE *f,*r;
f=fopen(PATH,"wb");if(!f||setvbuf(f,NULL,_IOLBF,8)||setvbuf(stdin,NULL,_IONBF,0))return 1;
fputs("prompt",f);if(getchar()!='X')return 2;
r=fopen(PATH,"rb");if(!r||fgetc(r)!='p'||fclose(r))return 3;
return fclose(f)||remove(PATH);}
'@) 0 $false 'X'
# Check a file from a second self-compiled process after the first returns.
Check 'stdio-default-file-return' ($prefix + @'
int main(void){FILE *f=fopen(PATH,"wb");if(!f||fputs("flushed",f))return 1;return 0;}
'@)
Check 'stdio-default-file-return-observe' ($prefix + @'
int main(void){FILE *f=fopen(PATH,"rb");char b[8];
if(!f||fread(b,1,8,f)!=7||memcmp(b,"flushed",7))return 1;
return fclose(f)||remove(PATH);}
'@)
