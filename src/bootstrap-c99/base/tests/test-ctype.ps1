$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $runtime = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../lib/runtime.c'))
    $prefix = "#include <ctype.h>`n"
    $suffix = "`n#include `"$runtime`"`n"
    # Independent test oracle: explicit ASCII character sets, emitted as C test data only.
    $upper = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    $lower = 'abcdefghijklmnopqrstuvwxyz'
    $digits = '0123456789'
    $punctuation = '!"#$%&''()*+,-./:;<=>?@[\]^_`{|}~'
    $sets = [ordered]@{
        isalnum = $upper + $lower + $digits
        isalpha = $upper + $lower
        isblank = " `t"
        iscntrl = ( -join ((0..31) + 127 | ForEach-Object { [char]$_ }))
        isdigit = $digits
        isgraph = $upper + $lower + $digits + $punctuation
        islower = $lower
        isprint = ' ' + $upper + $lower + $digits + $punctuation
        ispunct = $punctuation
        isspace = " `t`n`v`f`r"
        isupper = $upper
        isxdigit = $digits + 'ABCDEFabcdef'
    }
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        foreach ($entry in $sets.GetEnumerator()) {
            $bits = '0' + ( -join (0..255 | ForEach-Object { if ($entry.Value.IndexOf([char]$_) -ge 0) {
                            '1'
                        }
                        else {
                            '0'
                        } }))
            $driver = 'int main(void){int i;int (*f)(int)=' + $entry.Key + ';char *want="' + $bits + '";for(i=-1;i<256;i++){if((f(i)!=0)!=(want[i+1]==''1''))return 1;if((' + $entry.Key + '(i)!=0)!=(want[i+1]==''1''))return 2;}return 0;}'
            Check "ctype-$mode-$($entry.Key)-all-bytes" ($prefix + $driver + $suffix)
        }
        foreach ($direction in 'tolower', 'toupper') {
            $from = if ($direction -eq 'tolower') {
                $upper
            }
            else {
                $lower
            }
            $to = if ($direction -eq 'tolower') {
                $lower
            }
            else {
                $upper
            }
            $values = @(-1) + @(0..255 | ForEach-Object { $index = $from.IndexOf([char]$_); if ($index -ge 0) {
                        [int][char]$to[$index]
                    }
                    else {
                        $_
                    } })
            $driver = 'int expected[]={' + ($values -join ',') + '};int main(void){int i;int (*f)(int)=' + $direction + ';for(i=-1;i<256;i++){if(f(i)!=expected[i+1])return 1;if(' + $direction + '(i)!=expected[i+1])return 2;}return 0;}'
            Check "ctype-$mode-$direction-all-bytes" ($prefix + $driver + $suffix)
        }
    }
    [Environment]::SetEnvironmentVariable('C1_STANDARD', 'c89', 'Process')
    Check 'ctype-repeated-header' ($prefix + $prefix + 'int main(void){return isalpha(''A'') && isdigit(''3'');}' + $suffix) 1
    Check 'ctype-implementation-before-caller' ($prefix + $suffix + 'int main(void){return toupper(''a'');}') 65
    $body = 'int main(void){int n=65;'
    foreach ($functionName in @($sets.Keys) + @('tolower', 'toupper')) {
        $body += $functionName + '(n++);'
    }
    $body += 'return n==79;}'
    Check 'ctype-single-evaluation' ($prefix + $body + $suffix) 1
    Check 'ctype-text-transform' ($prefix + 'int main(void){char *p="Abc_09-zZ!";int i;for(i=0;p[i];i++)__c0_putchar(toupper((unsigned char)p[i]));return 0;}' + $suffix) 0 $false '' 'ABC_09-ZZ!'
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}
