foreach ($n in 63, 64, 127, 1024, 4095) {
    $name = 'v' + ('x' * ($n - 1))
    Check "identifier-local-$n" ("int main(void){int $name=42;return $name!=42;}") 0
    Check "identifier-global-$n" ("int $name=42;int main(void){return $name!=42;}") 0
    Check "identifier-function-$n" ("int $name(void){return 42;}int main(void){return $name()!=42;}") 0
}
$prefix = 'a' * 62
$a = $prefix + 'X' + ('p' * 32)
$b = $prefix + 'Y' + ('p' * 32)
Check 'identifier-significant-63' ("int main(void){int $a=20,$b=22;return $a+$b!=42;}") 0
$name = 'z' * 127
Check 'identifier-macro-full-spelling' ("#define STR_I(x) #x`n#define STR(x) STR_I(x)`nint main(void){return sizeof(STR($name))!=128;}") 0
Check 'identifier-paste-full-spelling' ("#define CAT(a,b) a ## b`nint $name=42;int main(void){return CAT(z," + ('z' * 126) + ')!=42;}') 0
Check 'identifier-typedef-long' ("typedef int $name;int main(void){$name v=42;return v!=42;}") 0
Check 'identifier-member-long' ("struct S{int $name;};int main(void){struct S s;s.$name=42;return s.$name!=42;}") 0
foreach ($n in 64, 127, 4095) {
    $name = 'f' + ('x' * ($n - 1))
    Check "identifier-func-spelling-$n" ("int $name(void){int i;char *expected=`"$name`";if(sizeof(__func__)!=" + ($n + 1) + ")return 1;for(i=0;i<=$n;i++)if(__func__[i]!=expected[i])return 2;return 0;}int main(void){return $name();}") 0
}
