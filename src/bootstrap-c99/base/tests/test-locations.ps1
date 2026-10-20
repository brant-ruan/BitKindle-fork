# Source-location and line-control checks, using the shared preprocessing helpers.
PpCheck 'line-first' @('int main(void){return __LINE__;}') 1
PpCheck 'line-blank-lines' @('', '', 'int main(void){return __LINE__;}') 3
PpCheck 'line-comment-newlines' @('/* one', 'two', 'three */', 'int main(void){return __LINE__;}') 4
Check 'line-crlf' "`r`nint main(void){return __LINE__;}`r`n" 2
Check 'line-cr' "`rint main(void){return __LINE__;}`r" 2
Check 'line-spliced' ('int main(void){return ' + [char]92 + [char]10 + '__LINE__;}') 2
Check 'line-trigraph-spliced' ('int main(void){return ??/' + [char]13 + [char]10 + '__LINE__;}') 2
Check 'line-two-splices' ('int main(void){return ' + [char]92 + [char]10 + [char]92 + [char]10 + '__LINE__;}') 3
PpCheck 'line-object-use-site' @('#define L __LINE__', '', 'int main(void){return L;}') 3
PpCheck 'line-function-use-site' @('#define L() __LINE__', '', 'int main(void){return L();}') 3
PpCheck 'line-function-lookahead' @('#define L() __LINE__', 'int main(void){return L(', '', ');}') 2
PpCheck 'line-argument-location' @('#define ID(x) x', 'int main(void){return ID(', '__LINE__', ');}') 3
PpCheck 'line-body-and-argument' @('#define F(x) __LINE__+(x)', 'int main(void){return F(', '__LINE__', ');}') 5
PpCheck 'line-object-in-argument' @('#define L __LINE__', '#define ID(x) x', 'int main(void){return ID(', 'L', ');}') 4
PpCheck 'line-nested-expansion' @('#define A() __LINE__', '#define B() A()', 'int main(void){return B();}') 3
PpCheck 'line-variadic-argument' @('#define V(...) __VA_ARGS__', 'int main(void){return V(', '__LINE__', ');}') 3
PpCheck 'line-paste-use-site' @('#define CAT(a,b) a ## b', 'int main(void){return CAT(', '__LI,', 'NE__', ');}') 2
PpString 'line-stringize-unexpanded' @('#define S(x) #x') 'S(__LINE__)' '__LINE__'
PpString 'line-stringize-expanded' @('#define S(x) #x', '#define E(x) S(x)') 'E(__LINE__)' '3'
PpCheck 'line-defined' @('#if !defined(__LINE__) || !defined __FILE__', '#error missing', '#endif', 'int main(void){return 0;}')
PpCheck 'line-ifdef' @('#ifdef __LINE__', 'int main(void){return 7;}', '#else', '#error missing', '#endif') 7
PpCheck 'file-ifndef' @('#ifndef __FILE__', '#error missing', '#endif', 'int main(void){return 0;}')
PpCheck 'line-if-evaluation' @('#if __LINE__ == 1', 'int main(void){return 9;}', '#else', '#error wrong', '#endif') 9
PpString 'file-stdin' @() '__FILE__' '<stdin>'
PpString 'file-macro-use-site' @('#define FILE __FILE__') 'FILE' '<stdin>'
PpString 'file-paste' @('#define CAT(a,b) a ## b') 'CAT(__FI,LE__)' '<stdin>'
PpCheck 'line-control-basic' @('#line 37', 'int main(void){return __LINE__;}') 37
PpCheck 'line-control-decimal-leading-zero' @('#line 00037', 'int main(void){return __LINE__;}') 37
PpCheck 'line-control-increments' @('#line 37', '', 'int main(void){return __LINE__;}') 38
PpCheck 'line-control-reset' @('#line 37', '#line 9', 'int main(void){return __LINE__;}') 9
PpCheck 'line-control-macro' @('#define N 41', '#line N', 'int main(void){return __LINE__;}') 41
PpCheck 'line-control-function-macro' @('#define ID(x) x', '#line ID(43)', 'int main(void){return __LINE__;}') 43
PpCheck 'line-control-current-line' @('#line __LINE__', 'int main(void){return __LINE__;}') 1
PpCheck 'line-control-max' @('#line 2147483647', 'int main(void){return __LINE__==2147483647;}') 1
PpCheck 'line-control-after-max' @('#line 2147483647', 'int main(void){', 'return __LINE__==2147483648LL;', '}') 1
PpString 'line-control-current-file' @('#line 20 "previous.c"', '#line 30 __FILE__') '__FILE__' 'previous.c'
PpCheck 'line-control-comment-newline' @('#line 60 /*', 'comment */', 'int main(void){return __LINE__;}') 60
PpCheck 'line-control-inactive' @('#if 0', '#line 1000', '#endif', 'int main(void){return __LINE__;}') 4
PpString 'file-line-control' @('#line 17 "virtual.c"') '__FILE__' 'virtual.c'
PpString 'file-line-control-empty' @('#line 17 ""') '__FILE__' ''
PpString 'file-line-control-preserved' @('#line 17 "virtual.c"', '#line 3') '__FILE__' 'virtual.c'
PpString 'file-line-control-macro' @('#define NAME "virtual.c"', '#line 17 NAME') '__FILE__' 'virtual.c'
PpString 'file-line-control-generated-string' @('#define S(x) #x', '#line 17 S(virtual.c)') '__FILE__' 'virtual.c'
PpString 'file-line-control-escaped' @('#line 17 "dir\\virtual.c"') '__FILE__' 'dir\virtual.c'
PpCheck 'line-control-spliced-directive' @(('#line ' + [char]92), '57', 'int main(void){return __LINE__;}') 57
$locHeader = Join-Path $ScratchDirectory 'location-header.h'
$locHeaderName = $locHeader.Replace('\', '/')
$locInclude = '#include "' + $locHeaderName + '"'
[IO.File]::WriteAllText($locHeader, "#define HEADER_LINE __LINE__`n#define HEADER_FILE __FILE__`nint header_line=__LINE__;", [Text.Encoding]::ASCII)
PpCheck 'line-header-physical' @($locInclude, 'int main(void){return header_line;}') 3
PpCheck 'line-header-macro-use-site' @($locInclude, 'int main(void){return HEADER_LINE;}') 2
PpString 'file-header-macro-use-site' @($locInclude) 'HEADER_FILE' '<stdin>'
[IO.File]::WriteAllText($locHeader, 'char *header_file=__FILE__;', [Text.Encoding]::ASCII)
PpString 'file-header-location' @($locInclude) 'header_file' $locHeaderName
[IO.File]::WriteAllText($locHeader, "#line 77 `"virtual-header.c`"`nint header_line=__LINE__;`nchar *header_file=__FILE__;", [Text.Encoding]::ASCII)
PpCheck 'line-header-control' @($locInclude, 'int main(void){return header_line;}') 77
PpCheck 'line-header-restores-parent' @('#line 20 "parent.c"', $locInclude, 'int main(void){return __LINE__;}') 21
PpString 'file-header-restores-parent' @('#line 20 "parent.c"', $locInclude) '__FILE__' 'parent.c'
PpString 'file-header-control' @($locInclude) 'header_file' 'virtual-header.c'
[IO.File]::WriteAllText($locHeader, ('int header_line=' + [char]92 + [char]10 + '__LINE__;'), [Text.Encoding]::ASCII)
PpCheck 'line-header-splice' @($locInclude, 'int main(void){return header_line;}') 2
$locChild = Join-Path $ScratchDirectory 'location-child.h'
[IO.File]::WriteAllText($locChild, 'int child_line=__LINE__;', [Text.Encoding]::ASCII)
[IO.File]::WriteAllText($locHeader, "#line 50 `"display-only.c`"`n#include `"location-child.h`"`nint parent_line=__LINE__;", [Text.Encoding]::ASCII)
PpCheck 'line-nested-header-return' @($locInclude, 'int main(void){return child_line+parent_line;}') 52
PpCheck 'line-high-physical-index' ((@('') * 300) + @('int main(void){return __LINE__==301;}')) 1
[IO.File]::WriteAllText($locHeader, "#define F(x) __LINE__+(x)`nint cross=F(", [Text.Encoding]::ASCII)
PpCheck 'line-call-crosses-header-eof' @($locInclude, '__LINE__);', 'int main(void){return cross;}') 4
[IO.File]::WriteAllText($locHeader, "#define F(x) __FILE__`nchar *cross_file=F(", [Text.Encoding]::ASCII)
PpString 'file-call-crosses-header-eof' @($locInclude, '7);') 'cross_file' $locHeaderName
Check 'line-spliced-identifier' ('int main(void){return __LI' + [char]92 + [char]10 + 'NE__;}') 1
PpCheck 'line-reject-zero' @('#line 0', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-overflow' @('#line 2147483648', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-huge-number' @('#line 9999999999999999999999999999', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-hex' @('#line 0x10', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-suffix' @('#line 10U', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-expression' @('#line 2+3', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-negative' @('#line -1', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-missing' @('#line', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-wide-filename' @('#line 3 L"wide.c"', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-extra-token' @('#line 3 "one.c" "two.c"', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-redefinition' @('#define __LINE__ 7', 'int main(void){return 0;}') 0 $true
PpCheck 'file-reject-redefinition' @('#define __FILE__ "x"', 'int main(void){return 0;}') 0 $true
PpCheck 'line-reject-undef' @('#undef __LINE__', 'int main(void){return 0;}') 0 $true
PpCheck 'file-reject-undef' @('#undef __FILE__', 'int main(void){return 0;}') 0 $true
