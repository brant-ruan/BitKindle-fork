$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $fixture = Join-Path $ScratchDirectory ('fixtures-' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory $fixture | Out-Null
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n"
    $empty = Join-Path $fixture 'empty'
    New-Item -ItemType Directory $empty | Out-Null
    $p = $empty.Replace('\', '/')
    Check 'remove-empty-directory' ($prefix + "int main(void){errno=EDOM;if(remove(`"$p`"))return 1;return errno!=EDOM;}") 0
    if (Test-Path -LiteralPath $empty) {
        throw 'Empty directory remains'
    }
    $full = Join-Path $fixture 'full'
    New-Item -ItemType Directory $full | Out-Null
    $child = Join-Path $full 'keep.txt'
    [IO.File]::WriteAllText($child, 'preserve')
    $p = $full.Replace('\', '/')
    Check 'remove-nonempty-directory' ($prefix + "int main(void){return remove(`"$p`")==0;}") 0
    if ([IO.File]::ReadAllText($child) -cne 'preserve') {
        throw 'Nonempty directory content changed'
    }
    $file = Join-Path $fixture 'ordinary.txt'
    [IO.File]::WriteAllText($file, 'test')
    $p = $file.Replace('\', '/')
    Check 'remove-ordinary-file' ($prefix + "int main(void){return remove(`"$p`")!=0;}") 0
    if (Test-Path -LiteralPath $file) {
        throw 'Ordinary file remains'
    }
    Check 'remove-missing-file' ($prefix + "int main(void){errno=0;if(!remove(`"$p`"))return 1;return errno!=ENOENT;}") 0
    $readonly = Join-Path $fixture 'readonly.txt'
    [IO.File]::WriteAllText($readonly, 'preserve')
    [IO.File]::SetAttributes($readonly, [IO.FileAttributes]::ReadOnly)
    $p = $readonly.Replace('\', '/')
    try {
        Check 'remove-readonly-file' ($prefix + "int main(void){errno=0;if(!remove(`"$p`"))return 1;return errno!=EACCES;}") 0
        if ([IO.File]::ReadAllText($readonly) -cne 'preserve') {
            throw 'Readonly file changed'
        }
    }
    finally {
        [IO.File]::SetAttributes($readonly, [IO.FileAttributes]::Normal)
    }
    Check 'remove-null-extension' ($prefix + 'int main(void){errno=0;if(!remove(NULL))return 1;return errno!=EINVAL;}') 0
}
finally {
    if ($null -eq $savedInclude) {
        Remove-Item Env:C1_INCLUDE -ErrorAction SilentlyContinue
    }
    else {
        $env:C1_INCLUDE = $savedInclude
    }
}
