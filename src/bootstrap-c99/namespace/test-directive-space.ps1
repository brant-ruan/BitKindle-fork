param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$RequireDiagnostics = $true
$CompilerDirectory = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'host'
$testRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/directive-space'
New-Item -ItemType Directory -Force $testRoot | Out-Null
$ErrorActionPreference = 'Stop'
$vt = [char]11
$ff = [char]12
$header = Join-Path $testRoot 'leading-whitespace.h'
[IO.File]::WriteAllText($header, "${vt}${ff}int header_value=7;`n", [Text.Encoding]::ASCII)
$header = $header.Replace('\', '/')
$cases = [ordered]@{
    define_vt = @("#define${vt}N 7`nint main(void){return N!=7;}", 1)
    define_ff = @("#define N${ff}7`nint main(void){return N!=7;}", 1)
    if_vt = @("#if${vt}1`nint main(void){return 0;}`n#endif", 1)
    null_ff = @("#${ff}`nint main(void){return 0;}", 1)
    before_hash = @("${vt}#define N 7`nint main(void){return N!=7;}", 0)
    ordinary = @("int${ff}main(void){return 0;}", 0)
    comment = @("#define /*${vt}${ff}*/ N 7`nint main(void){return N!=7;}", 0)
    literal = @("#define S `"${vt}${ff}`"`nint main(void){return S[0]!=11||S[1]!=12;}", 0)
    include_header = @("#include `"$header`"`nint main(void){return header_value!=7;}", 0)
    include_trailing = @("#include `"$header`"${vt}`nint main(void){return header_value!=7;}", 1)
    skipped = @("#if 0`n#define${vt}N 7`n#endif`nint main(void){return 0;}", 0)
    following_line = @("#define N 7`nint${ff}main(void){return N!=7;}", 0)
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
                $needed = $cases[$name][1]
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
                        "PASS $mode generation $g $name rejected with diagnostic and no binary"
                        continue
                    }
                    else {
                        if ($stderr.Length) {
                            throw "Expected silent original acceptance $g $name"
                        }
                        "REPRO missing directive whitespace diagnostic generation $g $name"
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
    'PASS directive whitespace diagnostic probe complete'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
