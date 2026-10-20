param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$ObjectMode)
$build = [IO.Path]::GetFullPath($BuildDirectory)

$ErrorActionPreference = 'Stop'
$testRoot = if ($ObjectMode) {
    'diagnostic-object-tests'
}
else {
    'diagnostic-tests'
}
. (Join-Path $PSScriptRoot 'cases.ps1')
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
            $dir = Join-Path $PSScriptRoot "$testRoot/$g/$mode"
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
                $p = Start-Process -FilePath ((Join-Path $build "host/c1-stage$g.exe")) -WindowStyle Hidden -PassThru -RedirectStandardInput $source -RedirectStandardOutput $artifact -RedirectStandardError $err
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
                        $linker = (Join-Path $build "link-stage$g.exe")
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
                $a = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "$testRoot/1/$mode/$name.$ext"))
                $b = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "$testRoot/2/$mode/$name.$ext"))
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
        'PASS 18 object diagnostic cases and 18 nonempty equal artifact pairs'
    }
    else {
        'PASS 18 diagnostic cases and 9 nonempty equal executable pairs'
    }
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
