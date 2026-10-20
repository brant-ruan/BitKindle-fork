$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n"
    Check 'func-main-array' ($prefix + @'
int main(void){const char (*p)[5]=&__func__;if(sizeof(__func__)!=5||strcmp(*p,"main"))return 1;return 0;}
'@) 0
    Check 'func-static-lifetime' ($prefix + @'
static const char *alpha(void){return __func__;}
static const char *beta(void){return __func__;}
int main(void){const char *a=alpha(),*b=beta();if(strcmp(a,"alpha")||strcmp(b,"beta")||a!=alpha()||a==b)return 1;return 0;}
'@) 0
    Check 'func-recursion' ($prefix + @'
static const char *recurse(int n){const char *p=__func__;if(n&&p!=recurse(n-1))return 0;return p;}
int main(void){const char *p=recurse(4);return !p||strcmp(p,"recurse");}
'@) 0
    Check 'func-nested-shadow' ($prefix + @'
int main(void){if(strcmp(__func__,"main"))return 1;{int __func__=17;if(__func__!=17)return 2;}if(strcmp(__func__,"main"))return 3;return 0;}
'@) 0
    Check 'func-oldstyle' ($prefix + @'
static const char *old(n) int n; {return n?__func__:0;}
int main(void){return strcmp(old(1),"old");}
'@) 0
    Check 'func-static-initializer' ($prefix + @'
static const char *f(void){static const char *p=__func__;return p;}
int main(void){return strcmp(f(),"f");}
'@) 0
    Check 'func-not-macro' ($prefix + @'
#ifdef __func__
#error must not be predefined as macro
#endif
int main(void){return sizeof(__func__)!=5;}
'@) 0
    Check 'func-reject-write' 'int main(void){__func__[0]=0;return 0;}' 0 $true
    Check 'func-reject-discard-const' 'int main(void){char *p=__func__;return 0;}' 0 $true
    Check 'func-reject-file-use' 'const char *p=__func__;int main(void){return 0;}' 0 $true
}
finally {
    $env:C1_INCLUDE = $savedInclude
}
