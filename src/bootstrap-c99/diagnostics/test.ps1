param([Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$ObjectMode)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$ErrorActionPreference = 'Stop'
$cases = [ordered]@{
    width = @('(0xffffffffUL+1UL)==4294967296UL', $false, $false, 0)
    highdecimal = @('9223372036854775808 > 0', $false, $true, 0)
    maxdecimal = @('18446744073709551615 == 0xffffffffffffffffUL', $false, $true, 0)
    unsignedcompare = @('!(-1 < 0xffffffffffffffffUL)', $false, $false, 0)
    signedboundary = @('-1 < 0x7fffffffffffffff', $false, $false, 0)
    wrap = @('(0xffffffffffffffffUL+1UL)==0', $false, $false, 0)
    longlong = @('42LL == 42', $false, $false, 1)
    overflow = @('18446744073709551616 > 0', $true, $true, 0)
    signedLLoverflow = @('9223372036854775808LL > 0', $true, $true, 0)
    unsignedLLmax = @('18446744073709551615ULL > 0', $false, $false, 1)
}
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_RUNTIME', 'C1_INCLUDE') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = $null
    foreach ($g in 1, 2) {
        foreach ($mode in 'c89', 'c99') {
            $env:C1_STANDARD = $mode
            $dir = Join-Path $BuildDirectory "tests/$g/$mode"
            New-Item -ItemType Directory -Force $dir | Out-Null
            foreach ($name in $cases.Keys) {
                $source = Join-Path $dir "$name.c"
                $exe = Join-Path $dir "$name.exe"
                $err = Join-Path $dir "$name.stderr"
                [IO.File]::WriteAllText($source, ("#if " + $cases[$name][0] + "`nint main(void){return 0;}`n#else`n#error incorrect preprocessing value`n#endif`n"), [Text.Encoding]::ASCII)
                $p = Start-Process -FilePath (Join-Path $BuildDirectory "host/c1-stage$g.exe") -WindowStyle Hidden -PassThru -RedirectStandardInput $source -RedirectStandardOutput $exe -RedirectStandardError $err
                [void]$p.Handle
                if (!$p.WaitForExit(30000)) {
                    $p.Kill()
                    throw 'Compile timeout'
                }
                $p.WaitForExit()
                $reject = $cases[$name][$(if ($mode -eq 'c89') {
                        1
                    }
                    else {
                        2
                    })]
                if ($p.ExitCode -ne [int]$reject) {
                    throw "Wrong result: $mode $name"
                }
                $lines = Get-Content $err
                if ($reject) {
                    if ((Get-Item $exe).Length -or !@($lines | Where-Object { $_ -like 'C1 error:*' }).Count) {
                        throw 'Missing rejection diagnostic or output leaked'
                    }
                }
                else {
                    $warnings = if ($mode -eq 'c89') {
                        $cases[$name][3]
                    }
                    else {
                        0
                    }
                    if (@($lines).Count -ne $warnings -or @($lines | Where-Object { $_ -like 'C1 warning:*' }).Count -ne $warnings) {
                        throw "Unexpected diagnostic: $mode $name"
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
                "PASS generation $g $mode preprocessing $name"
            }
        }
    }
    $pairs = 0
    foreach ($mode in 'c89', 'c99') {
        foreach ($name in $cases.Keys) {
            if ($cases[$name][$(if ($mode -eq 'c89') {
                        1
                    }
                    else {
                        2
                    })]) {
                continue
            }
            $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "tests/1/$mode/$name.exe"))
            $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "tests/2/$mode/$name.exe"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Artifact length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Artifact bytes mismatch'
                }
            }
            $pairs++
        }
    }
    "PASS 40 preprocessing cases and $pairs nonempty equal executable pairs"
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
