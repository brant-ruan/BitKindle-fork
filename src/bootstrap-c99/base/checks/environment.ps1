$savedInclude = $env:C1_INCLUDE
$savedTestEnvironment = $script:TestEnvironment
$names = @('C1_ENV_TEST_VALUE', 'C1_ENV_TEST_TARGET', 'C1_ENV_TEST_LONG', 'C1_ENV_TEST_MISSING', 'C1_ENV_TEST_EMPTY')
$saved = @{}
foreach ($name in $names) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include') + ';' + [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $env:C1_ENV_TEST_VALUE = 'C1_ENV_TEST_TARGET'
    $env:C1_ENV_TEST_TARGET = 'value with spaces = 17'
    $env:C1_ENV_TEST_LONG = 'z' * 30000
    Remove-Item -LiteralPath Env:C1_ENV_TEST_MISSING -ErrorAction SilentlyContinue
    # CLR 4 removes empty parent values; inject an empty value into the child block.
    $script:TestEnvironment = @{'C1_ENV_TEST_EMPTY' = '' }
    $prefix = @'
#include "src/bootstrap-c99/base/lib/runtime.c"
#include "src/bootstrap-c99/base/lib/environment.c"
'@
    Check 'environment-basic' ($prefix + "`n" + @'
int main(void){char *p=getenv("C1_ENV_TEST_TARGET");if(!p||strcmp(p,"value with spaces = 17"))return 1;return 0;}
'@) 0
    Check 'environment-absent' ($prefix + "`n" + @'
int main(void){if(getenv("C1_ENV_TEST_MISSING")||getenv(""))return 1;return 0;}
'@) 0
    Check 'environment-long' ($prefix + "`n" + @'
int main(void){char *p=getenv("C1_ENV_TEST_LONG");int i;if(!p||strlen(p)!=30000)return 1;for(i=0;i<30000;i++)if(p[i]!='z')return 2;return 0;}
'@) 0
    Check 'environment-aliased-name' ($prefix + "`n" + @'
int main(void){char *p=getenv("C1_ENV_TEST_VALUE");if(!p)return 1;p=getenv(p);if(!p||strcmp(p,"value with spaces = 17"))return 2;return 0;}
'@) 0
    Check 'environment-repeat-case-errno' ($prefix + "`n" + @'
int main(void){int i;char *p;errno=EDOM;for(i=0;i<64;i++){p=getenv("c1_env_test_target");if(!p||strcmp(p,"value with spaces = 17"))return 1;p=getenv("C1_ENV_TEST_LONG");if(!p||strlen(p)!=30000)return 2;}if(errno!=EDOM)return 3;return 0;}
'@) 0
    Check 'environment-empty' ($prefix + "`n" + @'
int main(void){char *p=getenv("C1_ENV_TEST_EMPTY");if(!p||*p)return 1;return 0;}
'@) 0
}
finally {
    $script:TestEnvironment = $savedTestEnvironment
    $env:C1_INCLUDE = $savedInclude
    foreach ($name in $names) {
        if ($null -eq $saved[$name]) {
            Remove-Item -LiteralPath ('Env:' + $name) -ErrorAction SilentlyContinue
        }
        else {
            [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process')
        }
    }
}
