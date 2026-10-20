function DiagnosticCheck([string]$Name, [string]$Source, [string]$Expected) {
    Check $Name $Source 0 $true
    $actual = [IO.File]::ReadAllText((Join-Path $ScratchDirectory "$Name.err"))
    if ($actual -cne ($Expected + "`n")) {
        throw "$Name diagnostic mismatch: <$actual> expected <$Expected>"
    }
}
DiagnosticCheck 'diagnostic-error-empty' '#error' 'C1 error: "<stdin>":1: #error'
DiagnosticCheck 'diagnostic-error-text' '#error missing configuration' 'C1 error: "<stdin>":1: #error missing configuration'
DiagnosticCheck 'diagnostic-error-no-expansion' "#define VALUE 123`n#error VALUE" 'C1 error: "<stdin>":2: #error VALUE'
DiagnosticCheck 'diagnostic-error-comment' '#error a/**/b + c' 'C1 error: "<stdin>":1: #error a b + c'
DiagnosticCheck 'diagnostic-error-spelling' '#error f(x)+"quoted\\name"' 'C1 error: "<stdin>":1: #error f(x)+"quoted\\name"'
DiagnosticCheck 'diagnostic-error-line' "#line 73 `"logical.c`"`n#error fail" 'C1 error: "logical.c":73: #error fail'
DiagnosticCheck 'diagnostic-error-splice' "`n#error first \`nsecond" 'C1 error: "<stdin>":2: #error first second'
DiagnosticCheck 'diagnostic-error-crlf' "`r`n`r`n#error fail`r`n" 'C1 error: "<stdin>":3: #error fail'
DiagnosticCheck 'diagnostic-error-trigraph' '??=error failure' 'C1 error: "<stdin>":1: #error failure'
DiagnosticCheck 'diagnostic-error-digraph' '%:error failure' 'C1 error: "<stdin>":1: #error failure'
DiagnosticCheck 'diagnostic-error-large-line' "#line 2147483647`n`n#error fail" 'C1 error: "<stdin>":2147483648: #error fail'
DiagnosticCheck 'diagnostic-error-first-only' "#error first`n#error second" 'C1 error: "<stdin>":1: #error first'
DiagnosticCheck 'diagnostic-error-buffer' ('#error ' + ('a ' * 5000).TrimEnd()) ('C1 error: "<stdin>":1: #error ' + ('a ' * 5000).TrimEnd())
DiagnosticCheck 'diagnostic-preprocessor' '#unknown' 'C1 error: "<stdin>":1: preprocessing failed'
DiagnosticCheck 'diagnostic-parser' 'int main(void){return ;}' 'C1 error: invalid or unsupported translation unit'
DiagnosticCheck 'diagnostic-lexer' 'int main(void){return 09;}' 'C1 error: invalid or unsupported translation unit'
DiagnosticCheck 'diagnostic-source-limit' (' ' * 1048577) 'C1 error: source input exceeds 1048576 bytes'
PpCheck 'diagnostic-inactive-error' @('#if 0', '#error not emitted', '#endif', 'int main(void){return 0;}')
if ((Get-Item (Join-Path $ScratchDirectory 'diagnostic-inactive-error.err')).Length) {
    throw 'Inactive error emitted a diagnostic'
}
$header = Join-Path $ScratchDirectory 'diagnostic-header.h'
[IO.File]::WriteAllText($header, "#line 19 `"nested.h`"`n#error header failure", [Text.Encoding]::ASCII)
DiagnosticCheck 'diagnostic-header-error' ('#include "' + $header + '"') 'C1 error: "nested.h":19: #error header failure'
[IO.File]::WriteAllText($header, "#line 19 `"nested.h`"`nint value;", [Text.Encoding]::ASCII)
DiagnosticCheck 'diagnostic-header-return' ("#include `"$header`"`n#error root failure") 'C1 error: "<stdin>":2: #error root failure'
