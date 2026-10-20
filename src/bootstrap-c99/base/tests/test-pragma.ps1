PpCheck 'pragma-operator-basic' @('_Pragma("vendor ignored")', 'int main(void){return 3;}') 3
PpCheck 'pragma-operator-empty' @('_Pragma("")', 'int main(void){return 5;}') 5
PpCheck 'pragma-operator-wide' @('_Pragma(L"vendor ignored")', 'int main(void){return 7;}') 7
PpCheck 'pragma-operator-expression' @('int main(void){return 1+_Pragma("vendor ignored")2;}') 3
PpCheck 'pragma-operator-declaration' @('int _Pragma("vendor ignored") value=11;int main(void){return value;}') 11
PpCheck 'pragma-operator-multiline' @('_Pragma', '(', '"vendor ignored"', ')', 'int main(void){return 13;}') 13
PpCheck 'pragma-operator-comments' @('_Pragma/**/(/**/"vendor ignored"/**/)', 'int main(void){return 17;}') 17
PpCheck 'pragma-operand-object-macro' @('#define TEXT "vendor ignored"', '_Pragma(TEXT)', 'int main(void){return 19;}') 19
PpCheck 'pragma-operand-function-macro' @('#define ID(x) x', '_Pragma(ID("vendor ignored"))', 'int main(void){return 23;}') 23
PpCheck 'pragma-operator-name-macro' @('#define PR _Pragma', 'PR("vendor ignored")', 'int main(void){return 29;}') 29
PpCheck 'pragma-parentheses-macros' @('#define LP (', '#define RP )', '_Pragma LP "vendor ignored" RP', 'int main(void){return 31;}') 31
PpCheck 'pragma-complete-operand-macro' @('#define BODY ("vendor ignored")', '_Pragma BODY', 'int main(void){return 37;}') 37
PpCheck 'pragma-stringized-macro' @('#define DO(x) _Pragma(#x)', 'DO(vendor ignored)', 'int main(void){return 41;}') 41
PpCheck 'pragma-variadic-macro' @('#define DO(...) _Pragma(#__VA_ARGS__)', 'DO(vendor, ignored)', 'int main(void){return 43;}') 43
PpCheck 'pragma-pasted-operator' @('#define CAT(a,b) a ## b', 'CAT(_Pra,gma)("vendor ignored")', 'int main(void){return 47;}') 47
PpCheck 'pragma-nested-operator' @('_Pragma(_Pragma("inner")"outer")', 'int main(void){return 53;}') 53
PpCheck 'pragma-rescan-following-call' @('#define F(x) x+1', '#define ID(x) x', 'int main(void){return ID(_Pragma("vendor") F)(6);}') 7
PpCheck 'pragma-if-expression' @('#if _Pragma("vendor") 1', 'int main(void){return 59;}', '#else', '#error wrong', '#endif') 59
PpCheck 'pragma-inactive' @('#if 0', '_Pragma()', '#endif', 'int main(void){return 61;}') 61
PpCheck 'pragma-inactive-elif' @('#if 1', 'int main(void){return 67;}', '#elif _Pragma()', '#endif') 67
PpCheck 'pragma-not-defined-macro' @('#if defined(_Pragma)', '#error operator is not an object macro', '#endif', 'int main(void){return 0;}')
PpCheck 'pragma-quotes-destringized' @('_Pragma("listing on \"..\\listing.dir\"")', 'int main(void){return 71;}') 71
PpCheck 'pragma-no-hex-decoding' @('_Pragma("\x53TDC INVALID ON")', 'int main(void){return 73;}') 73
PpCheck 'pragma-no-octal-decoding' @('_Pragma("\123TDC INVALID ON")', 'int main(void){return 79;}') 79
PpCheck 'pragma-no-newline-decoding' @('_Pragma("\nSTDC INVALID ON")', 'int main(void){return 83;}') 83
PpCheck 'pragma-payload-comment' @('_Pragma("/* prefix */ STDC FP_CONTRACT ON /* tail */")', 'int main(void){return 89;}') 89
PpCheck 'pragma-does-not-create-directive' @('_Pragma("#define VALUE 7")', '#ifdef VALUE', '#error injected', '#endif', 'int main(void){return 0;}')
PpString 'pragma-stringized-not-executed' @('#define S(x) #x') 'S(_Pragma())' '_Pragma()'
PpString 'pragma-leading-space-preserved' @('#define S(x) #x', '#define E(x) S(x)') 'E(a _Pragma("vendor")b)' 'a b'
PpString 'pragma-no-invented-space' @('#define S(x) #x', '#define E(x) S(x)') 'E(a+_Pragma("vendor")b)' 'a+b'
PpString 'pragma-repeated-removal' @('#define S(x) #x', '#define E(x) S(x)') 'E(a _Pragma("one")_Pragma("two")b)' 'a b'
PpCheck 'pragma-unused-argument-not-executed' @('#define IGNORE(x) 7', 'int main(void){return IGNORE(_Pragma());}') 7
PpCheck 'pragma-unused-replacement' @('#define UNUSED _Pragma()', 'int main(void){return 0;}')
foreach ($pragmaName in 'FP_CONTRACT', 'FENV_ACCESS', 'CX_LIMITED_RANGE') {
    foreach ($pragmaSetting in 'ON', 'OFF', 'DEFAULT') {
        PpCheck "pragma-stdc-$pragmaName-$pragmaSetting" @(('_Pragma("STDC ' + $pragmaName + ' ' + $pragmaSetting + '")'), 'int main(void){return 0;}')
    }
}
PpCheck 'pragma-directive-stdc' @('#pragma STDC FP_CONTRACT ON', 'int main(void){return 0;}')
PpCheck 'pragma-stdc-does-not-expand' @('#define ON BAD', '#pragma STDC FP_CONTRACT ON', '_Pragma("STDC FENV_ACCESS ON")', 'int main(void){return 0;}')
PpCheck 'pragma-unknown-does-not-expand' @('#define ALIAS STDC', '#pragma ALIAS INVALID BAD', 'int main(void){return 0;}')
PpCheck 'pragma-reject-no-parenthesis' @('_Pragma "vendor"', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-no-string' @('_Pragma()', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-number' @('_Pragma(7)', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-character' @('_Pragma(''x'')', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-adjacent-strings' @('_Pragma("one" "two")', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-unclosed' @('_Pragma("vendor"', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-extra-operand' @('_Pragma("vendor",7)', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-extra-parentheses' @('_Pragma(("vendor"))', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-expanded-number' @('#define ARG 7', '_Pragma(ARG)', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-payload-comment' @('_Pragma("vendor /* unfinished")', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-stdc-name' @('_Pragma("STDC INVALID ON")', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-stdc-setting' @('_Pragma("STDC FP_CONTRACT BAD")', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-stdc-missing-setting' @('_Pragma("STDC FP_CONTRACT")', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-stdc-extra-token' @('_Pragma("STDC FP_CONTRACT ON extra")', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-directive-stdc' @('#pragma STDC FP_CONTRACT BAD', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-definition' @('#define _Pragma(x) x', 'int main(void){return 0;}') 0 $true
PpCheck 'pragma-reject-undef' @('#undef _Pragma', 'int main(void){return 0;}') 0 $true
PpCheck 'defined-reject-definition' @('#define defined(x) 1', 'int main(void){return 0;}') 0 $true
PpCheck 'defined-reject-undef' @('#undef defined', 'int main(void){return 0;}') 0 $true
$pragmaHeader = Join-Path $ScratchDirectory 'pragma-header.h'
[IO.File]::WriteAllText($pragmaHeader, '#define HEADER_PRAGMA(x) _Pragma(#x)', [Text.Encoding]::ASCII)
PpCheck 'pragma-header-definition' @(('#include "' + $pragmaHeader.Replace('\', '/') + '"'), 'HEADER_PRAGMA(vendor ignored)', 'int main(void){return 0;}')
PpCheck 'pragma-line-after-removal' @('_Pragma("vendor")', 'int main(void){return __LINE__;}') 2
PpCheck 'pragma-node-reuse' @('#define P _Pragma("vendor")', (('P ' * 2000) + 'int main(void){return 0;}'))
