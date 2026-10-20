function ReasonCheck([string]$Name, [string]$Source, [string]$Reason) {
    Check $Name $Source 0 $true
    $message = [IO.File]::ReadAllText((Join-Path $ScratchDirectory "$Name.err"))
    if (!$message.EndsWith(': ' + $Reason + "`n")) {
        throw "$Name wrong reason: $message"
    }
    if (([regex]::Matches($message, 'C1 error: ')).Count -ne 1) {
        throw "$Name duplicate diagnostic"
    }
}
ReasonCheck 'reason-define-name' '#define 1 x' 'expected macro name after define'
ReasonCheck 'reason-define-reserved' '#define __LINE__ 3' 'cannot define or undefine reserved preprocessing name'
ReasonCheck 'reason-undef-reserved' '#undef _Pragma' 'cannot define or undefine reserved preprocessing name'
ReasonCheck 'reason-macro-parameter' '#define F(1) x' 'expected macro parameter name'
ReasonCheck 'reason-macro-duplicate' '#define F(x,x) x' 'duplicate macro parameter'
ReasonCheck 'reason-macro-parenthesis' '#define F(x y) x' 'expected closing parenthesis in macro parameters'
ReasonCheck 'reason-paste-first' '#define F(x) ## x' 'token paste cannot begin or end replacement list'
ReasonCheck 'reason-paste-last' '#define F(x) x ##' 'token paste cannot begin or end replacement list'
ReasonCheck 'reason-stringize-number' '#define F(x) # 1' 'stringize requires macro parameter'
ReasonCheck 'reason-stringize-name' '#define F(x) # y' 'stringize requires macro parameter'
ReasonCheck 'reason-redefinition-body' "#define F 1`n#define F 2" 'incompatible macro redefinition'
ReasonCheck 'reason-redefinition-arity' "#define F(x) x`n#define F(x,y) x" 'incompatible macro redefinition'
ReasonCheck 'reason-redefinition-parameter' "#define F(x) x`n#define F(y) y" 'incompatible macro redefinition'
ReasonCheck 'reason-pragma-open' '_Pragma "x"' 'expected opening parenthesis after _Pragma'
ReasonCheck 'reason-pragma-string' '_Pragma(1)' 'expected string literal in _Pragma'
ReasonCheck 'reason-pragma-close' '_Pragma("x" 1)' 'expected closing parenthesis after _Pragma string'
ReasonCheck 'reason-pragma-macro' "#define F(x) _Pragma(x)`nF(1)" 'expected string literal in _Pragma'
$missing = Join-Path $ScratchDirectory 'reason-no-such-header.h'
if (Test-Path -LiteralPath $missing) {
    throw 'Missing header fixture unexpectedly exists'
}
ReasonCheck 'reason-include-missing' ('#include "' + $missing + '"') 'cannot open include file'
