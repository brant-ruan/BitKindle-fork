$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    Check 'wide-character-type' '#include <stddef.h>
int main(void){wchar_t c=L''A'';return sizeof(L''A'')!=sizeof(wchar_t)||c!=65||sizeof(+L''A'')!=sizeof(int);}' 0
    Check 'wide-character-escapes' 'int main(void){return L''\n''!=10||L''\777''!=511||L''\xFFFF''!=65535||L''\0''!=0;}' 0
    Check 'wide-string-type' '#include <stddef.h>
int main(void){wchar_t *p=L"AB";wchar_t (*q)[3]=&L"AB";return sizeof(L"AB")!=6||p[0]!=65||p[1]!=66||p[2]||(*q)[1]!=66;}' 0
    Check 'wide-empty' 'int main(void){return sizeof(L"")!=2||L""[0];}' 0
    Check 'wide-embedded-null' 'int main(void){return sizeof(L"A\0B")!=8||L"A\0B"[1]||L"A\0B"[2]!=66||L"A\0B"[3];}' 0
    Check 'wide-join' 'int main(void){return sizeof(L"a" "b" L"c")!=8||("a" L"b")[1]!=98||sizeof("a" L"b")!=6;}' 0
    Check 'wide-local-inference' '#include <stddef.h>
int main(void){wchar_t a[]=L"\x1234Z";return sizeof(a)!=6||a[0]!=4660||a[1]!=90||a[2];}' 0
    Check 'wide-global-inference' '#include <stddef.h>
wchar_t a[]=L"AB";int main(void){return sizeof(a)!=6||a[0]!=65||a[1]!=66||a[2];}' 0
    Check 'wide-exact-array' '#include <stddef.h>
int main(void){wchar_t a[2]=L"AB";return a[0]!=65||a[1]!=66;}' 0
    Check 'wide-zero-fill' '#include <stddef.h>
wchar_t a[5]=L"AB";int main(void){wchar_t b[5]={L"CD"};return a[2]||a[3]||a[4]||b[0]!=67||b[1]!=68||b[2]||b[3]||b[4];}' 0
    Check 'wide-nested-arrays' '#include <stddef.h>
wchar_t a[][3]={L"ab",L"cd"};int main(void){wchar_t b[2][3]={L"ef",L"gh"};return sizeof(a)!=12||a[1][1]!=100||b[1][1]!=104||b[1][2];}' 0
    Check 'wide-record-inference' '#include <stddef.h>
struct S{int n;wchar_t v[3];};int main(void){struct S a[]={{3,L"ab"},{4,L"cd"}};return sizeof(a)/sizeof(a[0])!=2||a[1].n!=4||a[1].v[1]!=100;}' 0
    Check 'wide-static-pointer' '#include <stddef.h>
static wchar_t *p=L"abc"+1;static wchar_t (*q)[3]=&L"xy";int main(void){return *p!=98||p[1]!=99||p[2]||(*q)[1]!=121||((size_t)p&1);}' 0
    Check 'wide-static-local' '#include <stddef.h>
int f(void){static wchar_t a[]=L"A";return a[0]++;}int main(void){return f()!=65||f()!=66;}' 0
    Check 'wide-macro-paste' '#define W(s) L ## s
int main(void){return sizeof(W("ab"))!=6||W(''x'')!=120;}' 0
    Check 'wide-conditional-constant' '#if L''A'' != 65
#error wrong wide character constant
#endif
int main(void){switch(511){case L''\777'':return 0;}return 1;}' 0
    Check 'wide-format-ascii' '#include "src/bootstrap-c99/base/lib/runtime.c"
int main(void){return printf("%ls",L"wide")!=4;}' 0 $false '' 'wide'
    $allUnits = [Text.StringBuilder]::new()
    for ($unit = 0; $unit -le 65535; $unit++) {
        [void]$allUnits.Append(('\x{0:X4}' -f $unit))
    }
    $allSource = 'static unsigned short *p=L"' + $allUnits.ToString() + '";int main(void){int i;for(i=0;i<65536;i++)if(p[i]!=i)return 1;return p[65536];}'
    Check 'wide-all-65536-code-units' $allSource 0
    Check 'wide-reject-overflow' 'int main(void){return L''\x10000'';}' 0 $true
    Check 'wide-multiple-characters-last-unit' 'int main(void){return L''ab''!=98;}' 0
    Check 'wide-reject-small-array' 'unsigned short a[1]=L"ab";int main(void){return 0;}' 0 $true
    Check 'wide-reject-char-array' 'char a[]=L"ab";int main(void){return 0;}' 0 $true
    Check 'wide-reject-narrow-to-wide' 'unsigned short a[]="ab";int main(void){return 0;}' 0 $true
    Check 'wide-reject-pointer-type' 'int main(void){char *p=L"ab";return 0;}' 0 $true
    Check 'wide-reject-spaced-prefix' 'int main(void){return sizeof(L "ab");}' 0 $true
}
finally {
    if ($null -eq $savedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $savedInclude
    }
}
