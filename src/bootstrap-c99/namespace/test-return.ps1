param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$Original = $false
$testRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/bare-return'
$ErrorActionPreference = 'Stop'
$compilerRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'host'
$tag = if ($Original) {
    'original'
}
else {
    'fixed'
}
$cases = [ordered]@{}
foreach ($type in 'int', 'double', 'int *', 'struct Small', 'struct Large') {
    $name = $type.Replace(' ', '_').Replace('*', 'pointer')
    $cases[$name] = @("struct Small{int n;};struct Large{int n[10];};int count; $type f(void){count++;return;count+=100;}int main(void){f();f();return count!=2;}", 1)
}
$cases['implicit'] = @('int count;f(){count++;return;}int main(void){f();return count!=1;}', 1)
$cases['void'] = @('int count;void f(void){count++;return;}int main(void){f();return count!=1;}', 0)
$cases['void_value'] = @('void f(void){return 3;}int main(void){f();return 0;}', 2)
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($mode in 'c89', 'c99') {
        foreach ($g in 1, 2) {
            $env:C1_STANDARD = $mode
            $dir = Join-Path $testRoot "$tag-$mode-$g"
            New-Item -ItemType Directory -Force $dir | Out-Null
            foreach ($name in $cases.Keys) {
                $body = $cases[$name][0]
                $kind = $cases[$name][1]
                $reject = $kind -eq 2 -or ($kind -eq 1 -and ($Original -or $mode -eq 'c99'))
                $src = Join-Path $dir "$name.c"
                $exe = Join-Path $dir "$name.exe"
                $err = Join-Path $dir "$name.stderr"
                [IO.File]::WriteAllText($src, $body, [Text.Encoding]::ASCII)
                $info = [Diagnostics.ProcessStartInfo]::new([IO.Path]::GetFullPath((Join-Path $compilerRoot "c1-stage$g.exe")))
                $info.UseShellExecute = $false
                $info.CreateNoWindow = $true
                $info.RedirectStandardInput = $true
                $info.RedirectStandardOutput = $true
                $info.RedirectStandardError = $true
                $p = [Diagnostics.Process]::new()
                $p.StartInfo = $info
                $out = [IO.File]::Create($exe)
                try {
                    [void]$p.Start()
                    $output = $p.StandardOutput.BaseStream.CopyToAsync($out)
                    $stderr = $p.StandardError.ReadToEndAsync()
                    $p.StandardInput.Write($body)
                    $p.StandardInput.Close()
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Compiler timeout'
                    }
                    [void]$output.GetAwaiter().GetResult()
                    $message = $stderr.GetAwaiter().GetResult()
                    [IO.File]::WriteAllText($err, $message)
                    $expected = if ($reject) {
                        1
                    }
                    else {
                        0
                    }
                    if ($p.ExitCode -ne $expected) {
                        throw "Compile status $tag $mode $g $name"
                    }
                }
                finally {
                    $out.Dispose()
                    $p.Dispose()
                }
                if ($reject) {
                    if ((Get-Item $exe).Length -or !$message.Contains('C1 error:')) {
                        throw 'Missing diagnostic or nonempty rejection output'
                    }
                    "PASS $tag $mode generation $g $name diagnosed rejection"
                }
                else {
                    if ($message.Length) {
                        throw 'Unexpected diagnostic'
                    }
                    $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
                    [void]$p.Handle
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Client timeout'
                    }
                    $p.WaitForExit()
                    if ($p.ExitCode) {
                        throw "Runtime failed $name"
                    }
                    "PASS $tag $mode generation $g $name discarded return and side effects"
                }
            }
        }
    }
    foreach ($mode in 'c89', 'c99') {
        foreach ($name in $cases.Keys) {
            $a = [IO.File]::ReadAllBytes((Join-Path $testRoot "$tag-$mode-1/$name.exe"))
            $b = [IO.File]::ReadAllBytes((Join-Path $testRoot "$tag-$mode-2/$name.exe"))
            if ($a.Length -ne $b.Length) {
                throw 'Pair length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Pair bytes mismatch'
                }
            }
        }
    }
    "PASS $tag bare return mode tests and generation artifact equality"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
