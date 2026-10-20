# Sourced after the preprocessing test helpers have been defined.
PpCheck 'va-identity' @('#define V(...) __VA_ARGS__', 'int main(void){return V(17);}') 17
PpCheck 'va-empty' @('#define V(...) __VA_ARGS__', 'int main(void){V() return 3;}') 3
PpCheck 'va-array-list' @('#define V(...) __VA_ARGS__', 'int main(void){int a[]={V(2,7,9)};return sizeof(a)+a[1];}') 19
PpCheck 'va-fixed-and-list' @('#define V(x,...) x+(__VA_ARGS__)', 'int main(void){return V(3,2,8);}') 11
PpCheck 'va-fixed-empty-tail' @('#define V(x,...) x __VA_ARGS__', 'int main(void){return V(13,);}') 13
PpCheck 'va-empty-fixed' @('#define V(x,...) x __VA_ARGS__', 'int main(void){return V(,21);}') 21
PpCheck 'va-all-empty' @('#define V(x,...) x __VA_ARGS__', 'int main(void){V(,) return 4;}') 4
PpCheck 'va-prescan' @('#define N 11', '#define V(...) (__VA_ARGS__)', 'int main(void){return V(N+3);}') 14
PpCheck 'va-prescan-comma' @('#define PAIR 4,6', '#define V(...) __VA_ARGS__', 'int main(void){int a[]={V(PAIR)};return a[0]+a[1];}') 10
PpCheck 'va-nested-call' @('#define V(...) (__VA_ARGS__)', 'int main(void){return V(2+V(7));}') 9
PpCheck 'va-forward-call' @('#define APPLY(f,...) f(__VA_ARGS__)', 'int f(int a,int b){return a*10+b;}', 'int main(void){return APPLY(f,2,3);}') 23
PpCheck 'va-forward-empty' @('#define APPLY(f,...) f(__VA_ARGS__)', 'int f(void){return 29;}', 'int main(void){return APPLY(f,);}') 29
PpCheck 'va-forward-macro' @('#define SUM(a,b) ((a)+(b))', '#define APPLY(f,...) f(__VA_ARGS__)', 'int main(void){return APPLY(SUM,5,6);}') 11
PpCheck 'va-if-expression' @('#define V(...) (__VA_ARGS__)', '#if V(1+2)==3', 'int main(void){return 31;}', '#else', '#error wrong', '#endif') 31
PpCheck 'va-paste-left' @('#define V(...) item ## __VA_ARGS__', 'int main(void){int item7=37;return V(7);}') 37
PpCheck 'va-paste-right' @('#define V(...) __VA_ARGS__ ## tail', 'int main(void){int headtail=41;return V(head);}') 41
PpCheck 'va-paste-empty' @('#define V(...) item ## __VA_ARGS__', 'int main(void){int item=43;return V();}') 43
PpCheck 'va-paste-fixed-empty' @('#define V(x,...) x ## __VA_ARGS__', 'int main(void){int item=47;return V(item,);}') 47
PpCheck 'va-paste-list-edge' @('#define V(...) item ## __VA_ARGS__', 'int main(void){int item7=3;int a[]={V(7,9)};return a[0]+a[1];}') 12
PpCheck 'va-redefine-identical' @('#define V(x,...) x+(__VA_ARGS__)', '#define V(x, ...) x+(__VA_ARGS__)', 'int main(void){return V(2,8);}') 10
PpCheck 'va-redefine-after-undef' @('#define V(x,...) x', '#undef V', '#define V(x) x+1', 'int main(void){return V(5);}') 6
PpCheck 'va-redefine-from-fixed' @('#define V(x) x', '#undef V', '#define V(...) (__VA_ARGS__)', 'int main(void){return V(3,7);}') 7
PpCheck 'va-cross-line' @('#define V(...) (__VA_ARGS__)', 'int main(void){return V(', '2+', '3', ');}') 5
PpCheck 'va-no-call' @('#define V(...) 7', 'int V=5;int main(void){return V;}') 5
PpCheck 'va-following-call' @('#define V(...) __VA_ARGS__', '#define ADD(x) ((x)+1)', 'int main(void){return V(ADD)(6);}') 7
PpString 'va-stringize' @('#define S(...) #__VA_ARGS__') 'S(alpha, beta, gamma)' 'alpha, beta, gamma'
PpString 'va-stringize-empty' @('#define S(...) #__VA_ARGS__') 'S()' ''
PpString 'va-stringize-empty-elements' @('#define S(...) #__VA_ARGS__') 'S(, ,)' ', ,'
PpString 'va-stringize-no-prescan' @('#define X 7', '#define S(...) #__VA_ARGS__') 'S(X, X)' 'X, X'
PpString 'va-stringize-prescan' @('#define X 7', '#define S(...) #__VA_ARGS__', '#define EXPAND(...) S(__VA_ARGS__)') 'EXPAND(X, X)' '7, 7'
PpString 'va-stringize-nested-parentheses' @('#define S(...) #__VA_ARGS__') 'S((1,2),f(3,4))' '(1,2),f(3,4)'
PpString 'va-stringize-comments' @('#define S(...) #__VA_ARGS__') 'S(a/**/,/**/b)' 'a , b'
PpString 'va-stringize-digraph' @('#define S(...) %: __VA_ARGS__') 'S(a,b)' 'a,b'
PpString 'va-rescan-self' @('#define V(...) V(__VA_ARGS__)', '#define S(...) #__VA_ARGS__', '#define E(...) S(__VA_ARGS__)') 'E(V(1,2))' 'V(1,2)'
PpString 'va-rescan-mutual' @('#define A(...) B(__VA_ARGS__)', '#define B(...) A(__VA_ARGS__)', '#define S(...) #__VA_ARGS__', '#define E(...) S(__VA_ARGS__)') 'E(A(1,2))' 'A(1,2)'
PpString 'va-reserved-in-string' @('#define V(...) "__VA_ARGS__"') 'V(7)' '__VA_ARGS__'
PpString 'va-stringize-does-not-prescan-invalid-call' @('#define NEED(x,y) x', '#define S(...) #__VA_ARGS__') 'S(NEED(1))' 'NEED(1)'
PpString 'va-empty-pack-keeps-comma' @('#define V(x,...) x,__VA_ARGS__', '#define S(...) #__VA_ARGS__', '#define E(...) S(__VA_ARGS__)') 'E(V(a,))' 'a,'
PpCheck 'va-unused-pack-does-not-prescan' @('#define NEED(x,y) x', '#define V(...) 17', 'int main(void){return V(NEED(1));}') 17
PpCheck 'va-reject-gnu-comma-paste' @('#define V(x,...) x , ## __VA_ARGS__', 'int main(void){int a[]={V(1,2)};return 0;}') 0 $true
$vaHeader = Join-Path $ScratchDirectory 'variadic-header.h'
[IO.File]::WriteAllText($vaHeader, ('#define FROM_HEADER(...) (__VA_ARGS__)'), [Text.Encoding]::ASCII)
$vaInclude = '#include "' + $vaHeader.Replace('\', '/') + '"'
PpCheck 'va-header-definition' @($vaInclude, 'int main(void){return FROM_HEADER(3,19);}') 19
PpCheck 'va-header-restores-reserved-check' @($vaInclude, 'int __VA_ARGS__;int main(void){return 0;}') 0 $true
PpCheck 'va-reject-missing-tail' @('#define V(x,...) x', 'int main(void){return V(7);}') 0 $true
PpCheck 'va-reject-missing-fixed' @('#define V(x,y,...) x', 'int main(void){return V(7,);}') 0 $true
PpCheck 'va-reject-nonfinal-ellipsis' @('#define V(...,x) x', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-double-ellipsis' @('#define V(...,...) 7', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-named-gnu' @('#define V(args...) args', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-no-comma' @('#define V(x...) x', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-trailing-comma-definition' @('#define V(x,...,) x', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-reserved-parameter' @('#define V(__VA_ARGS__) 7', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-reserved-macro-name' @('#define __VA_ARGS__ 7', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-reserved-object-body' @('#define V __VA_ARGS__', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-reserved-fixed-body' @('#define V(x) __VA_ARGS__', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-reserved-source' @('int __VA_ARGS__;int main(void){return 0;}') 0 $true
PpCheck 'va-reject-reserved-unused-argument' @('#define V(...) 7', 'int main(void){return V(__VA_ARGS__);}') 0 $true
PpCheck 'va-reject-generated-reserved' @('#define CAT(a,b) a ## b', 'int CAT(__VA_,ARGS__);int main(void){return 0;}') 0 $true
PpCheck 'va-reject-redefinition-kind' @('#define V(x) 7', '#define V(...) 7', 'int main(void){return 0;}') 0 $true
PpCheck 'va-reject-redefinition-fixed-count' @('#define V(...) 7', '#define V(x,...) 7', 'int main(void){return 0;}') 0 $true
PpCheck 'va-inactive-reserved' @('#if 0', '__VA_ARGS__', '#define V __VA_ARGS__', '#endif', 'int main(void){return 0;}')
$vaNames = (0..126 | ForEach-Object { 'p' + $_ }) -join ','
$vaValues = (0..126 | ForEach-Object { '1' }) -join ','
PpCheck 'va-127-fixed-parameters' @(('#define V(' + $vaNames + ',...) p0+p126+(__VA_ARGS__)'), ('int main(void){return V(' + $vaValues + ',7);}')) 9
$vaTooMany = (0..127 | ForEach-Object { 'p' + $_ }) -join ','
PpCheck 'va-reject-128-fixed-parameters' @(('#define V(' + $vaTooMany + ',...) 7'), 'int main(void){return 0;}') 0 $true
$vaMany = (0..299 | ForEach-Object { '1' }) -join ','
PpCheck 'va-many-variable-arguments' @('#define V(...) __VA_ARGS__', ('int main(void){int a[]={V(' + $vaMany + ')};return sizeof(a)==1200;}')) 1
