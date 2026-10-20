param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$RequireDiagnostics = $true
$CompilerDirectory = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'host'
$testRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/aggregate'
$ErrorActionPreference = 'Stop'
$cases = [ordered]@{
    array = @('int main(void){int n=7;int a[1]={n};return a[0]!=7;}', 1)
    call = @('int f(void){return 7;}int main(void){int a[1]={f()};return a[0]!=7;}', 1)
    record = @('struct S{int n;};int main(void){int n=7;struct S s={n};return s.n!=7;}', 1)
    bitfield = @('struct S{unsigned int n:4;};int main(void){unsigned int n=7;struct S s={n};return s.n!=7;}', 1)
    union = @('union U{int n;double d;};int main(void){int n=7;union U u={n};return u.n!=7;}', 1)
    nestedcopy = @('struct S{int n;};struct T{struct S s;};int main(void){struct S s={7};struct T t={s};return t.s.n!=7;}', 1)
    scalar = @('int f(void){return 7;}int main(void){int n={f()};return n!=7;}', 0)
    recordcopy = @('struct S{int n;};struct S f(void){struct S s={7};return s;}int main(void){struct S s=f();return s.n!=7;}', 0)
    constant = @('int n=7;int main(void){int *a[1]={&n};double b[1]={1.5+2.5};char s[3]="ok";return *a[0]!=7||b[0]!=4||s[1]!=107;}', 0)
    elided = @('struct S{int a[2];};int main(void){struct S s={3,4};return s.a[0]+s.a[1]!=7;}', 0)
}
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    foreach ($g in 1, 2) {
        foreach ($mode in 'c89', 'c99') {
            $env:C1_STANDARD = $mode
            $dir = Join-Path $testRoot "$mode-$g"
            New-Item -ItemType Directory -Force $dir | Out-Null
            foreach ($name in $cases.Keys) {
                $needed = $mode -eq 'c89' -and $cases[$name][1]
                $src = Join-Path $dir "$name.c"
                $exe = Join-Path $dir "$name.exe"
                [IO.File]::WriteAllText($src, $cases[$name][0], [Text.Encoding]::ASCII)
                $info = [Diagnostics.ProcessStartInfo]::new([IO.Path]::GetFullPath((Join-Path $CompilerDirectory "c1-stage$g.exe")))
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
                    $read = $p.StandardOutput.BaseStream.CopyToAsync($out)
                    $err = $p.StandardError.ReadToEndAsync()
                    $p.StandardInput.Write($cases[$name][0])
                    $p.StandardInput.Close()
                    if (!$p.WaitForExit(30000)) {
                        $p.Kill()
                        throw 'Compiler timeout'
                    }
                    [void]$read.GetAwaiter().GetResult()
                    $stderr = $err.GetAwaiter().GetResult()
                    [IO.File]::WriteAllText((Join-Path $dir "$name.stderr"), $stderr)
                    $expected = if ($RequireDiagnostics -and $needed) {
                        1
                    }
                    else {
                        0
                    }
                    if ($p.ExitCode -ne $expected) {
                        throw "Unexpected compile exit $($p.ExitCode): $mode $g $name"
                    }
                }
                finally {
                    $out.Dispose()
                    $p.Dispose()
                }
                if ($needed) {
                    if ($RequireDiagnostics) {
                        if (!$stderr.Contains('C1 error:') -or (Get-Item $exe).Length) {
                            throw "Missing diagnostic or invalid binary output $g $name"
                        }
                        "PASS c89 generation $g $name rejected with diagnostic and no binary"
                        continue
                    }
                    else {
                        if ($stderr.Length) {
                            throw "Expected silent original acceptance $g $name"
                        }
                        "REPRO missing C89 aggregate diagnostic generation $g $name"
                    }
                }
                elseif ($stderr.Length) {
                    throw "Unexpected diagnostic $mode $g $name"
                }
                $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
                [void]$p.Handle
                if (!$p.WaitForExit(30000)) {
                    $p.Kill()
                    throw 'Client timeout'
                }
                $p.WaitForExit()
                if ($p.ExitCode) {
                    throw "Execution failed $mode $g $name"
                }
                "PASS $mode generation $g $name runtime"
            }
        }
    }
    'PASS aggregate constant diagnostic probe complete'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
