$savedInclude = $env:C1_INCLUDE
$savedValue = $env:C1_SYSTEM_TEST_VALUE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $env:C1_SYSTEM_TEST_VALUE = 'inherited value'
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n"
    Check 'system-windows-layout' ($prefix + 'int main(void){return sizeof(struct __c1_startup)!=104||offsetof(struct __c1_startup,flags)!=60||offsetof(struct __c1_startup,reserved_bytes)!=72||offsetof(struct __c1_startup,input)!=80||sizeof(struct __c1_process)!=24;}') 0
    Check 'system-query-and-exit' ($prefix + 'int main(void){if(!system(NULL))return 1;if(system("exit /b 37")!=37)return 2;return system("exit /b 201")!=201;}') 0
    Check 'system-empty-command' ($prefix + 'int main(void){return system("")!=0;}') 0
    Check 'system-stdout' ($prefix + 'int main(void){return system("echo hello")!=0;}') 0 $false '' "hello`r`n"
    Check 'system-quotes' ($prefix + 'int main(void){return system("echo \"two words\"")!=0;}') 0 $false '' "`"two words`"`r`n"
    Check 'system-stderr' ($prefix + 'int main(void){return system("echo error>&2")!=0;}') 0 $false '' '' "error`r`n"
    Check 'system-environment' ($prefix + 'int main(void){char *p=getenv("C1_SYSTEM_TEST_VALUE");if(!p)return 1;if(system("echo %C1_SYSTEM_TEST_VALUE%"))return 2;return strcmp(p,"inherited value")!=0;}') 0 $false '' "inherited value`r`n"
    Check 'system-standard-input' ($prefix + 'int main(void){return system("set /p C1_SYSTEM_READ=")!=0;}') 0 $false "input line`r`n"
    Check 'system-missing-command' ($prefix + 'int main(void){int r=system("c1_nonexistent_command_826914 2>nul");return r==0||r==-1;}') 0
    Check 'system-repeat' ($prefix + 'int main(void){int i;for(i=0;i<24;i++)if(system("exit /b 19")!=19)return 1;return 0;}') 0
    Check 'system-too-long' ($prefix + 'int main(void){char *p=malloc(32769);int result;if(!p)return 1;memset(p,32,32768);p[32768]=0;errno=0;result=system(p);free(p);return result!=-1||errno!=EINVAL;}') 0
    Check 'system child with spaces' 'int main(void){return 23;}' 23
    $child = (Join-Path $ScratchDirectory 'system child with spaces.exe').Replace('\', '/')
    $source = $prefix + 'int main(void){return system("\"' + $child + '\"")!=23;}'
    Check 'system-quoted-executable' $source 0
}
finally {
    if ($null -eq $savedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $savedInclude
    }
    if ($null -eq $savedValue) {
        Remove-Item Env:C1_SYSTEM_TEST_VALUE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_SYSTEM_TEST_VALUE = $savedValue
    }
}
