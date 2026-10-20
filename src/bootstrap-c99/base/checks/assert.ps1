$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/signal.c`"`n#include `"src/bootstrap-c99/base/lib/assert.c`"`n#include <assert.h>`n"
    Check 'assert-true-single-evaluation' ($prefix + @'
int main(void){int n=0;int *p=&n;assert(++n);assert(p);assert(1.5);if(n!=1)return 1;if(1)assert(n);else return 2;return 0;}
'@) 0
    Check 'assert-disabled-no-library' @'
#define NDEBUG 0
#include <assert.h>
int main(void){int n=0;assert(++n);assert(unknown_name());return n;}
'@ 0
    Check 'assert-inclusion-switches' ($prefix + @'
static int n;
static void a(void){assert(++n);}
#define NDEBUG
#include <assert.h>
static void b(void){assert(++n);}
#undef NDEBUG
#include <assert.h>
static void c(void){assert(++n);}
int main(void){a();b();c();return n==2?0:1;}
'@) 0
    Check 'assert-failure-diagnostic' ($prefix + @'
#line 70 "assert-case.c"
int main(void){assert(2 == 3);return 7;}
'@) 3 $false '' '' "Assertion failed: 2 == 3, file assert-case.c, line 70, function main`r`n"
    Check 'assert-unexpanded-text' ($prefix + @'
#define FALSE_VALUE 0
#line 101 "macro-case.c"
int main(void){assert(FALSE_VALUE);return 7;}
'@) 3 $false '' '' "Assertion failed: FALSE_VALUE, file macro-case.c, line 101, function main`r`n"
    Check 'assert-buffered-stderr' ($prefix + @'
#line 90 "buffered-case.c"
int main(void){char b[1024];setvbuf(stderr,b,_IOFBF,sizeof(b));assert(0);return 7;}
'@) 3 $false '' '' "Assertion failed: 0, file buffered-case.c, line 90, function main`r`n"
    Check 'assert-abort-handler-observes-once' ($prefix + @'
static int n;
static void h(int s){if(n==1&&s==SIGABRT)__c0_putchar('Y');}
#line 80 "once-case.c"
int main(void){signal(SIGABRT,h);assert(++n == 2);return 7;}
'@) 3 $false '' 'Y' "Assertion failed: ++n == 2, file once-case.c, line 80, function main`r`n"
    Check 'assert-reject-aggregate' ($prefix + @'
int main(void){struct S{int a;}s;assert(s);return 0;}
'@) 0 $true
}
finally {
    $env:C1_INCLUDE = $savedInclude
}
