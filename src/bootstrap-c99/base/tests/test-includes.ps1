# Sourced by test-c0.ps1 after Check and PpCheck are available.
$headerRoot = Join-Path $ScratchDirectory 'headers'
$headerNested = Join-Path $headerRoot 'nested'
$headerSystem = Join-Path $headerRoot 'system'
New-Item -ItemType Directory -Force -Path $headerRoot, $headerNested, $headerSystem | Out-Null
function Header([string]$Name, [string]$Text) {
    [IO.File]::WriteAllText((Join-Path $headerRoot $Name), $Text, [Text.Encoding]::ASCII)
}
function IncludeLine([string]$Name) {
    '#include "' + (Join-Path $headerRoot $Name).Replace('\', '/') + '"'
}
Header 'value.h' "#define VALUE 37`n"
PpCheck 'include-quoted-absolute' @((IncludeLine 'value.h'), 'int main(void){return VALUE;}') 37
Header 'empty.h' ''
PpCheck 'include-empty' @((IncludeLine 'empty.h'), 'int main(void){return 3;}') 3
Header 'tail.h' '#define TAIL 41'
PpCheck 'include-no-final-newline' @((IncludeLine 'tail.h'), 'int main(void){return TAIL;}') 41
Header 'decl.h' 'int from_header(void){return 19;}'
PpCheck 'include-definition' @((IncludeLine 'decl.h'), 'int main(void){return from_header();}') 19
Header 'nested/child.h' '#define CHILD 23'
Header 'parent.h' "#include `"nested/child.h`"`n#define PARENT (CHILD+5)"
PpCheck 'include-nested-relative' @((IncludeLine 'parent.h'), 'int main(void){return PARENT;}') 28
Header 'nested/up.h' '#include "../value.h"'
PpCheck 'include-parent-directory' @((IncludeLine 'nested/up.h'), 'int main(void){return VALUE;}') 37
Header 'guard.h' "#ifndef GUARD_H`n#define GUARD_H`n#include `"guard.h`"`nint guarded(void){return 11;}`n#endif"
PpCheck 'include-recursive-guard' @((IncludeLine 'guard.h'), (IncludeLine 'guard.h'), 'int main(void){return guarded();}') 11
Header 'use-parent.h' "#if SWITCH == 7`n#define CHOICE 31`n#else`n#error parent macro not visible`n#endif"
PpCheck 'include-parent-macro' @('#define SWITCH 7', (IncludeLine 'use-parent.h'), 'int main(void){return CHOICE;}') 31
Header 'undef.h' '#undef SWITCH'
PpCheck 'include-undef-parent' @('#define SWITCH 7', (IncludeLine 'undef.h'), '#ifdef SWITCH', '#error stale', '#endif', 'int main(void){return 0;}')
Header 'repeated.h' "#ifdef SEEN`n#undef RESULT`n#define RESULT 29`n#else`n#define SEEN`n#define RESULT 1`n#endif"
PpCheck 'include-repeat-expansion' @((IncludeLine 'repeated.h'), (IncludeLine 'repeated.h'), 'int main(void){return RESULT;}') 29
$quotedHeader = '"' + (Join-Path $headerRoot 'value.h').Replace('\', '/') + '"'
PpCheck 'include-object-macro' @(('#define FILE ' + $quotedHeader), '#include FILE', 'int main(void){return VALUE;}') 37
PpCheck 'include-function-macro' @('#define ID(x) x', ('#include ID(' + $quotedHeader + ')'), 'int main(void){return VALUE;}') 37
$unquotedHeader = (Join-Path $headerRoot 'value.h').Replace('\', '/')
PpCheck 'include-stringize' @('#define STR(x) #x', ('#include STR(' + $unquotedHeader + ')'), 'int main(void){return VALUE;}') 37
Header 'space name.h' '#define SPACE 17'
PpCheck 'include-space-filename' @((IncludeLine 'space name.h'), 'int main(void){return SPACE;}') 17
Header 'crlf.h' "#define CRLF 18`r`n"
Header 'cr.h' "#define CR 21`r"
PpCheck 'include-crlf' @((IncludeLine 'crlf.h'), 'int main(void){return CRLF;}') 18
PpCheck 'include-cr' @((IncludeLine 'cr.h'), 'int main(void){return CR;}') 21
Header 'phases.h' ('??=define PHASE (2??/' + [char]13 + [char]10 + '+8)' + [char]13 + [char]10)
PpCheck 'include-trigraph-splicing' @((IncludeLine 'phases.h'), 'int main(void){return PHASE;}') 10
Header 'conditional.h' "#if 1`n#define CONDITIONAL 33`n#endif"
PpCheck 'include-nested-conditional' @('#if 1', (IncludeLine 'conditional.h'), '#endif', 'int main(void){return CONDITIONAL;}') 33
PpCheck 'include-inactive-missing' @('#if 0', '#include "does-not-exist.h"', '#endif', 'int main(void){return 0;}')
PpCheck 'include-comment-tail' @(((IncludeLine 'value.h') + ' /* tail */'), 'int main(void){return VALUE;}') 37
Header 'retained.h' "#define TWICE(x) ((x)+(x))`n#define WORD hello`n#define STR(x) #x`n#define CAT(x,y) x ## y"
PpCheck 'include-retained-macro-spans' @((IncludeLine 'retained.h'), (IncludeLine 'value.h'), 'int main(void){int hello=4;return TWICE(CAT(hel,lo))+VALUE;}') 45
PpString 'include-retained-stringize' @((IncludeLine 'retained.h'), (IncludeLine 'value.h')) 'STR(one two)' 'one two'
Header 'system/sys.h' '#define SYSTEM 43'
Header 'system/value.h' '#define VALUE 9'
$oldInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = (Join-Path $headerRoot 'nonexistent') + ';' + $headerSystem
    PpCheck 'include-angle-search' @('#include <sys.h>', 'int main(void){return SYSTEM;}') 43
    PpCheck 'include-quoted-system-fallback' @('#include "sys.h"', 'int main(void){return SYSTEM;}') 43
    PpCheck 'include-angle-macro' @('#define FILE <sys.h>', '#include FILE', 'int main(void){return SYSTEM;}') 43
    PpCheck 'include-angle-macro-tokens' @('#define NAME sys', '#define FILE(x) <x.h>', '#include FILE(NAME)', 'int main(void){return SYSTEM;}') 43
    Header 'search.h' '#include "value.h"'
    PpCheck 'include-local-before-system' @((IncludeLine 'search.h'), 'int main(void){return VALUE;}') 37
    Header 'angle.h' '#include <value.h>'
    PpCheck 'include-angle-skips-local' @((IncludeLine 'angle.h'), 'int main(void){return VALUE;}') 9
    Header 'system/NAME.h' '#define LITERAL 47'
    PpCheck 'include-direct-angle-unexpanded' @('#define NAME other', '#include <NAME.h>', 'int main(void){return LITERAL;}') 47
    Header 'system/%literal.h' '#define PERCENT_HEADER 49'
    PpCheck 'include-angle-digraph-prefix' @('#include <%literal.h>', 'int main(void){return PERCENT_HEADER;}') 49
}
finally {
    $env:C1_INCLUDE = $oldInclude 
}
PpCheck 'include-reject-missing-operand' @('#include', 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-empty-name' @('#include ""', 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-empty-angle' @('#include <>', 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-extra-token' @(((IncludeLine 'value.h') + ' x'), 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-wide-string' @('#include L"value.h"', 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-expanded-extra' @(('#define FILE ' + $quotedHeader + ' x'), '#include FILE', 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-unclosed-angle' @('#include <abc', 'int main(void){return 0;}') 0 $true
Header 'bad-comment.h' '/* unfinished'
PpCheck 'include-reject-comment-eof' @((IncludeLine 'bad-comment.h'), 'int main(void){return 0;}') 0 $true
Header 'bad-open.h' '#if 1'
PpCheck 'include-reject-open-conditional' @((IncludeLine 'bad-open.h'), '#endif', 'int main(void){return 0;}') 0 $true
Header 'bad-close.h' '#endif'
PpCheck 'include-reject-close-parent' @('#if 1', (IncludeLine 'bad-close.h'), 'int main(void){return 0;}') 0 $true
Header 'bad-else.h' '#else'
PpCheck 'include-reject-parent-else' @('#if 1', (IncludeLine 'bad-else.h'), '#endif', 'int main(void){return 0;}') 0 $true
Header 'bad-elif.h' '#elif 0'
PpCheck 'include-reject-parent-elif' @('#if 1', (IncludeLine 'bad-elif.h'), '#endif', 'int main(void){return 0;}') 0 $true
Header 'recursive.h' '#include "recursive.h"'
PpCheck 'include-reject-depth-limit' @((IncludeLine 'recursive.h'), 'int main(void){return 0;}') 0 $true
PpCheck 'include-reject-name-limit' @(('#include "' + ('a' * 1024) + '"'), 'int main(void){return 0;}') 0 $true
Header 'too-large.h' (' ' * 1048576)
PpCheck 'include-reject-arena-limit' @((IncludeLine 'too-large.h'), 'int main(void){return 0;}') 0 $true
