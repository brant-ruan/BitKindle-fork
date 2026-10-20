# Inspect the compiler-produced bytes; never transform or patch an executable.
$operations = [ordered]@{
    'assignment' = @('x=2;', 0)
    'compound' = @('x+=2;', 1)
    'postfix' = @('x++;', 1)
    'prefix' = @('++x;', 1)
    'discard' = @('x;', 1)
    'void' = @('(void)x;', 1)
    'sizeof' = @('(void)sizeof(x);', 0)
    'twice' = @('x;x;', 2)
    'self-assignment' = @('x=x;', 1)
}
foreach ($type in 'float', 'double', 'long double') {
    foreach ($name in $operations.Keys) {
        $id = 'fp-volatile-' + $type.Replace(' ', '-') + '-' + $name
        $entry = $operations[$name]
        FloatingCheck $id ("int main(void){volatile $type x=1;" + $entry[0] + 'return 0;}')
        $bytes = [IO.File]::ReadAllBytes((Join-Path $ScratchDirectory ($id + '.exe')))
        $hex = [BitConverter]::ToString($bytes)
        $pattern = if ($type -eq 'float') {
            '8B-85-F8-FF-FF-FF'
        }
        else {
            '48-8B-85-F8-FF-FF-FF'
        }
        $reads = [regex]::Matches($hex, $pattern).Count
        if ($reads -ne $entry[1]) {
            throw "${id}: expected $($entry[1]) volatile reads, found $reads"
        }
    }
}
