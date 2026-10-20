$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n"
    $script:TestArguments = 'one "two three" ""'
    Check 'main-argc-argv' ($prefix + @'
int main(int argc,char **argv){if(argc!=4||argv[argc])return 1;if(!strstr(argv[0],"main-argc-argv.exe")||strcmp(argv[1],"one")||strcmp(argv[2],"two three")||*argv[3])return 2;argv[1][0]='O';argv[2]=argv[1];if(strcmp(argv[2],"One"))return 3;return 0;}
'@) 0
    Check 'main-array-parameter' ($prefix + @'
int main(int count,char *values[]){return count==4&&strcmp(values[2],"two three")==0?0:1;}
'@) 0
    Check 'main-oldstyle-parameters' ($prefix + @'
int main(count,values) int count;char **values;{return count==4&&strcmp(values[1],"one")==0?0:1;}
'@) 0
    Check 'main-prototype' ($prefix + @'
int main(int,char **);
int main(int n,char **p){return n==4&&p[4]==0?0:1;}
'@) 0
    Check 'main-void-with-arguments' 'int main(void){return 0;}' 0
    $script:TestArguments = ''
    Check 'main-no-extra-arguments' ($prefix + @'
int main(int n,char **v){if(n!=1||!v[0]||!*v[0]||v[1])return 1;return 0;}
'@) 0
    $script:TestArguments = 'survives'
    Check 'main-argv-lifetime-exit' ($prefix + @'
static char **saved;
static void done(void){printf("%s",saved[1]);}
int main(int n,char **v){if(n!=2)return 1;saved=v;atexit(done);return 29;}
'@) 29 $false '' 'survives'
    Check 'main-reject-one-parameter' 'int main(int n){return n;}' 0 $true
    Check 'main-reject-three-parameters' 'int main(int n,char **v,char **e){return 0;}' 0 $true
    Check 'main-reject-wrong-argv' 'int main(int n,char *v){return 0;}' 0 $true
    Check 'main-reject-wrong-argc' 'int main(long n,char **v){return 0;}' 0 $true
    Check 'main-reject-void-return' 'void main(void){}' 0 $true
    Check 'main-reject-variadic' 'int main(int n,char **v,...){return 0;}' 0 $true
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $script:TestArguments = ''
}
