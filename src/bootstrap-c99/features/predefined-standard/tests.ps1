$savedStandard = $env:C1_STANDARD
try {
    foreach ($standard in 'c89', 'c99') {
        $env:C1_STANDARD = $standard
        Check "predefined-$standard-values" "#if !defined(__STDC__) || __STDC__ != 1 || !defined(__STDC_HOSTED__) || __STDC_HOSTED__ != 1`n#error bad standard macros`n#endif`nint main(void){return __STDC__!=1||__STDC_HOSTED__!=1;}" 0
        Check "predefined-$standard-stringize" "#define S0(x) #x`n#define S(x) S0(x)`nint main(void){char *p=S(__STDC__);return p[0]!='1'||p[1];}" 0
        Check "predefined-$standard-paste" "#define C0(a,b) a##b`n#define C(a,b) C0(a,b)`nint value1=42;int main(void){return C(value,__STDC__)!=42;}" 0
        foreach ($name in '__STDC__', '__STDC_HOSTED__') {
            Check "predefined-$standard-undef-$name" "#undef $name`nint main(void){return 0;}" 0 $true
            Check "predefined-$standard-define-$name" "#define $name 0`nint main(void){return 0;}" 0 $true
        }
    }
    $env:C1_STANDARD = 'c99'
    Check 'predefined-c99-version' "#if !defined(__STDC_VERSION__) || __STDC_VERSION__ != 199901L`n#error bad version`n#endif`n#define S0(x) #x`n#define S(x) S0(x)`nint main(void){char *p=S(__STDC_VERSION__);return sizeof(S(__STDC_VERSION__))!=8||p[6]!='L'||__STDC_VERSION__!=199901L;}" 0
    Check 'predefined-c99-version-undef' "#undef __STDC_VERSION__`nint main(void){return 0;}" 0 $true
    $env:C1_STANDARD = 'c89'
    Check 'predefined-c89-no-version' "#ifdef __STDC_VERSION__`n#error c99 version in c89`n#endif`nint main(void){return 0;}" 0
}
finally {
    $env:C1_STANDARD = $savedStandard
}
