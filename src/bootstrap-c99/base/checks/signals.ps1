$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/signal.c`"`n"
    Check 'signal-install-reset' ($prefix + @'
static volatile sig_atomic_t value;
static void handler(int n){value=n;}
int main(void){if(signal(SIGTERM,handler)!=SIG_DFL)return 1;if(raise(SIGTERM)||value!=SIGTERM)return 2;if(signal(SIGTERM,SIG_IGN)!=SIG_DFL)return 3;return 0;}
'@) 0
    Check 'signal-ignore-and-replace' ($prefix + @'
static void h(int n){}
int main(void){int a[6]={SIGINT,SIGILL,SIGFPE,SIGSEGV,SIGTERM,SIGABRT};int i;for(i=0;i<6;i++){if(signal(a[i],SIG_IGN)!=SIG_DFL)return 1;if(raise(a[i])||raise(a[i]))return 2;if(signal(a[i],h)!=SIG_IGN)return 3;if(signal(a[i],SIG_DFL)!=h)return 4;}return 0;}
'@) 0
    Check 'signal-reinstall' ($prefix + @'
static int count;
static void h(int n){count++;signal(n,h);}
int main(void){signal(SIGINT,h);if(raise(SIGINT)||raise(SIGINT)||count!=2)return 1;if(signal(SIGINT,SIG_DFL)!=h)return 2;return 0;}
'@) 0
    Check 'signal-invalid-extension' ($prefix + @'
static void h(int n){}
int main(void){int a[5]={-1,0,1,23,2147483647};int i;signal(SIGTERM,h);for(i=0;i<5;i++){errno=0;if(signal(a[i],h)!=SIG_ERR||errno!=EINVAL)return 1;errno=0;if(raise(a[i])==0||errno!=EINVAL)return 2;}if(signal(SIGTERM,SIG_ERR)!=SIG_ERR)return 3;if(signal(SIGTERM,SIG_DFL)!=h)return 4;return 0;}
'@) 0
    Check 'signal-default-termination' ($prefix + @'
static void done(void){__c0_putchar('X');}
int main(void){atexit(done);fputs("buffered",stdout);raise(SIGTERM);return 7;}
'@) 143
    Check 'abort-default' ($prefix + @'
static void done(void){__c0_putchar('X');}
int main(void){atexit(done);fputs("buffered",stdout);abort();return 7;}
'@) 3
    Check 'abort-ignore' ($prefix + @'
int main(void){signal(SIGABRT,SIG_IGN);abort();return 7;}
'@) 3
    Check 'abort-handler-returns' ($prefix + @'
static void h(int n){__c0_putchar(n==SIGABRT?'Y':'N');}
int main(void){signal(SIGABRT,h);abort();return 7;}
'@) 3 $false '' 'Y'
    Check 'abort-handler-longjmp' ($prefix + @'
static jmp_buf b;
static void h(int n){longjmp(b,17);}
int main(void){int r=setjmp(b);if(!r){signal(SIGABRT,h);abort();return 1;}if(r!=17||signal(SIGABRT,SIG_IGN)!=SIG_DFL)return 2;return 0;}
'@) 0
    Check 'abort-handler-exit' ($prefix + @'
static void done(void){printf("done");}
static void h(int n){exit(37);}
int main(void){atexit(done);signal(SIGABRT,h);abort();return 7;}
'@) 37 $false '' 'done'
}
finally {
    $env:C1_INCLUDE = $savedInclude
}
