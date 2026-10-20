param([Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$ObjectMode)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)

$ErrorActionPreference = 'Stop'
$testRoot = if ($ObjectMode) {
    'skipped-object-tests'
}
else {
    'skipped-tests'
}
$cases = [ordered]@{
    skippedifdef = @("#if 0`n#ifdef 123 + junk`n#endif`n#endif`nint main(void){return 0;}", 0, $false)
    skippedifndef = @("#if 0`n#ifndef`n#endif`n#endif`nint main(void){return 0;}", 0, $false)
    skippedif = @("#if 0`n#if`n#endif`n#endif`nint main(void){return 0;}", 0, $false)
    skippedelse = @("#if 0`n#if 1`n#else extra tokens`n#endif`n#endif`nint main(void){return 0;}", 0, $false)
    skippedendif = @("#if 0`n#if 1`n#endif extra tokens`n#endif`nint main(void){return 0;}", 0, $false)
    resume = @("#if 0`n#ifndef 123`n#error hidden`n#endif`n#elif 1`nint main(void){return 0;}`n#else`n#error wrong branch`n#endif", 0, $false)
    skippedelif = @("#if 1`nint main(void){return 0;}`n#elif ( undefined +`n#error hidden`n#endif", 0, $false)
    nesteddefined = @("#define YES 1`n#if 0`n#ifdef YES`n#error hidden`n#else`n#error still hidden`n#endif`n#endif`nint main(void){return 0;}", 0, $false)
    activeifdef = @("#ifdef 123`n#endif`nint main(void){return 0;}", 0, $true)
    activeelse = @("#if 0`n#else extra`n#endif`nint main(void){return 0;}", 0, $true)
    activeendif = @("#if 0`n#endif extra`nint main(void){return 0;}", 0, $true)
    unclosed = @("#if 0`n#if 1`n#endif`nint main(void){return 0;}", 0, $true)
    partialtoken = @("#if 0`n#ifdef `"unterminated`n#endif`n#endif`nint main(void){return 0;}", 0, $true)
}
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_OUTPUT = if ($ObjectMode) {
        'object'
    }
    else {
        $null
    }
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($g in 1, 2) {
        foreach ($mode in 'c89', 'c99', 'default') {
            $env:C1_STANDARD = if ($mode -eq 'default') {
                $null
            }
            else {
                $mode
            }
            $dir = Join-Path $BuildDirectory "$testRoot/$g/$mode"
            New-Item -ItemType Directory -Force $dir | Out-Null
            foreach ($name in $cases.Keys) {
                $source = Join-Path $dir "$name.c"
                $exe = Join-Path $dir "$name.exe"
                $err = Join-Path $dir "$name.stderr"
                $body = $cases[$name][0]
                if ($ObjectMode -and $name -eq 'errorafter') {
                    $body = 'int main(void){;int n=0;return n+;}' 
                }
                [IO.File]::WriteAllText($source, $body, [Text.Encoding]::ASCII)
                $artifact = if ($ObjectMode) {
                    Join-Path $dir "$name.c1o"
                }
                else {
                    $exe
                }
                $p = Start-Process -FilePath (Join-Path $BuildDirectory "host/c1-stage$g.exe") -WindowStyle Hidden -PassThru -RedirectStandardInput $source -RedirectStandardOutput $artifact -RedirectStandardError $err
                [void]$p.Handle
                if (!$p.WaitForExit(30000)) {
                    $p.Kill()
                    throw 'Compiler timeout'
                }
                $p.WaitForExit()
                $expected = if ($cases[$name][2]) {
                    1
                }
                else {
                    0
                }
                if ($p.ExitCode -ne $expected) {
                    throw "Unexpected compile result: $name $mode"
                }
                $diagnostics = Get-Content $err
                $count = if ($mode -eq 'c89') {
                    $cases[$name][1]
                }
                else {
                    0
                }
                $warnings = @($diagnostics | Where-Object { $_ -like 'C1 warning: C99 extension in C89 mode: *' }).Count
                if (($count -ge 0 -and $warnings -ne $count) -or ($count -lt 0 -and $warnings -lt - $count)) {
                    throw "Wrong warning count: $name $mode"
                }
                if ($expected) {
                    if (@($diagnostics | Where-Object { $_ -like 'C1 error:*' }).Count -lt 1) {
                        throw 'Missing required preprocessing error'
                    }
                    if ((Get-Item $artifact).Length) {
                        throw 'Invalid source emitted binary bytes'
                    }
                }
                else {
                    if (@($diagnostics).Count -ne $warnings) {
                        throw 'Unexpected extra diagnostic'
                    }
                    if ($ObjectMode) {
                        $linker = (Join-Path $BuildDirectory "link-stage$g.exe")
                        $p = Start-Process -FilePath $linker -ArgumentList ('"' + $exe + '" main "' + $artifact + '"') -WindowStyle Hidden -PassThru
                        [void]$p.Handle
                        if (!$p.WaitForExit(30000)) {
                            $p.Kill()
                            throw 'Object link timeout'
                        }
                        $p.WaitForExit()
                        if ($p.ExitCode) {
                            throw 'Object link failed'
                        }
                    }
                    $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
                    [void]$p.Handle
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Execution timeout'
                    }
                    $p.WaitForExit()
                    if ($p.ExitCode) {
                        throw 'Execution failed'
                    }
                }
                "PASS generation $g $mode $name diagnostics and outcome"
            }
        }
    }
    $extensions = if ($ObjectMode) {
        @('exe', 'c1o')
    }
    else {
        @('exe')
    }
    foreach ($ext in $extensions) {
        foreach ($mode in 'c89', 'c99', 'default') {
            foreach ($name in ($cases.Keys | Where-Object { !$cases[$_][2] })) {
                $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "$testRoot/1/$mode/$name.$ext"))
                $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "$testRoot/2/$mode/$name.$ext"))
                if (!$a.Length -or $a.Length -ne $b.Length) {
                    throw 'Artifact length mismatch'
                }
                for ($i = 0; $i -lt $a.Length; $i++) {
                    if ($a[$i] -ne $b[$i]) {
                        throw 'Artifact mismatch'
                    }
                }
            }
        }
    }
    if ($ObjectMode) {
        'PASS 78 object diagnostic cases and 48 nonempty equal artifact pairs'
    }
    else {
        'PASS 78 diagnostic cases and 24 nonempty equal executable pairs'
    }
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
