param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$tools = if ($ToolDirectory) {
    [IO.Path]::GetFullPath($ToolDirectory)
}
else {
    [IO.Path]::GetFullPath($BuildDirectory)
}
. "$snapshot/tools/compile.ps1"
$dir = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/character-sign/raw'
New-Item -ItemType Directory -Force $dir | Out-Null
# Write source text in the compiler's documented single-byte source encoding.
# No executable or object is created by this source-generation step.
$body = "int main(void){`n"
foreach ($value in 128..255) {
    $c = [string][char]$value
    $expected = $value - 256
    $body += "if('$c'!=($expected))return 1;`n"
    $body += "if(L'$c'!=$value)return 2;`n"
    $body += "if((unsigned char)`"$c`"[0]!=$value)return 3;`n"
    $body += "if(L`"$c`"[0]!=$value)return 4;`n"
}
$body += "return 0;}`n"
$conditions = (128..255 | ForEach-Object { "#if '" + [string][char]$_ + "' != (" + ($_ - 256) + ")`n#error bad raw character sign`n#endif" }) -join "`n"
[IO.File]::WriteAllText((Join-Path $dir 'probe.c'), $conditions + "`n" + $body, [Text.Encoding]::GetEncoding(28591))
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = 'object'
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($mode in 'c89', 'c99') {
        $env:C1_STANDARD = $mode
        foreach ($g in 1, 2) {
            $obj = "$dir/$mode-$g.c1o"
            $exe = "$dir/$mode-$g.exe"
            Compile (Join-Path $tools "host/c1-stage$g.exe") "$dir/probe.c" $obj
            & "$tools/link-stage$g.exe" $exe __c1_start $obj "$tools/host/runtime$g.c1o" ($options -split ' ')
            if ($LASTEXITCODE) {
                throw 'Raw source link failed'
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Raw probe timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Raw source failed $mode generation $g : $($p.ExitCode)"
            }
            "PASS raw source $mode generation ${g}: 512 runtime and 128 preprocessing checks"
        }
        foreach ($ext in 'c1o', 'exe') {
            $a = [IO.File]::ReadAllBytes("$dir/$mode-1.$ext")
            $b = [IO.File]::ReadAllBytes("$dir/$mode-2.$ext")
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Raw artifact length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Raw artifact bytes mismatch'
                }
            }
            "PASS equal raw $mode $ext pair"
        }
    }
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
