param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$Original = $false
$ErrorActionPreference = 'Stop'
$build = [IO.Path]::GetFullPath($BuildDirectory)
$cases = [ordered]@{
    empty = ''
    whitespace = " `t`r`n"
    comment = '/* no declaration */'

    macro = "#define X 42`n"
    skipped = "#if 0`nint x;`n#endif`n"

    expansion = "#define EMPTY`nEMPTY`n"

    typedef = 'typedef int T;'
    tag = 'struct S;'
    prototype = 'int f(void);'
    external = 'extern int x;'
}
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = 'object'
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    $label = if ($Original) {
        'original'
    }
    else {
        'fixed'
    }
    foreach ($g in 1, 2) {
        foreach ($mode in 'c89', 'c99') {
            $env:C1_STANDARD = $mode
            $dir = Join-Path $build "namespace/empty-unit/$label/$mode-$g"
            New-Item -ItemType Directory -Force $dir | Out-Null
            $compiler = if ($Original) {
                "$build/host/c1-stage$g.exe"
            }
            else {
                "$build/host/c1-stage$g.exe"
            }
            foreach ($name in $cases.Keys) {
                $inputFile = Join-Path $dir "$name.c"
                $outputFile = Join-Path $dir "$name.c1o"
                $err = Join-Path $dir "$name.err"
                [IO.File]::WriteAllText($inputFile, $cases[$name], [Text.Encoding]::ASCII)
                $p = Start-Process -FilePath $compiler -WindowStyle Hidden -PassThru -RedirectStandardInput $inputFile -RedirectStandardOutput $outputFile -RedirectStandardError $err
                [void]$p.Handle
                if (!$p.WaitForExit(30000)) {
                    $p.Kill()
                    throw 'Compiler timeout'
                }
                $p.WaitForExit()
                $reject = !$Original -and $name -in @('empty', 'whitespace', 'comment', 'macro', 'skipped', 'expansion')
                if ($p.ExitCode -ne [int]$reject) {
                    throw "Unexpected exit: $label/$mode/$g/$name"
                }
                $diagnostic = [IO.File]::ReadAllText($err)
                if ($reject) {
                    if ((Get-Item $outputFile).Length -or $diagnostic -notmatch 'C1 error: empty translation unit') {
                        throw "Missing rejection: $name"
                    }
                }
                elseif (!(Get-Item $outputFile).Length -or $diagnostic.Length) {
                    throw "Unexpected output: $name"
                }
                "PASS $label $mode generation $g $name"
            }
        }
    }
    foreach ($mode in 'c89', 'c99') {
        foreach ($name in @('typedef', 'tag', 'prototype', 'external')) {
            if ((Get-FileHash "$build/namespace/empty-unit/$label/$mode-1/$name.c1o").Hash -ne (Get-FileHash "$build/namespace/empty-unit/$label/$mode-2/$name.c1o").Hash) {
                throw 'Object mismatch'
            }
        }
    }
    if ($Original) {
        'PASS reproduced 24 silent empty-unit acceptances; 16 valid declarations accepted'
    }
    else {
        'PASS 24 empty-unit diagnostic rejections; 16 valid declarations accepted; 8 equal object pairs'
    }
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
