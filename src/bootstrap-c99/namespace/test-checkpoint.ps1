param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$Fixed = $true
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
if ($ToolDirectory) {
    $build = [IO.Path]::GetFullPath($ToolDirectory)
}
$testRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/checkpoint'
New-Item -ItemType Directory -Force $testRoot | Out-Null
. (Join-Path $snapshot 'tools/compile.ps1')
$source = Join-Path $snapshot 'host/compiler.c'
$body = Get-Content $source -Raw
$body = [regex]::Replace($body, '(?m)^int main\(void\)', 'int compiler_original_main(void)')
$body += [char]10 + (Get-Content (Join-Path $PSScriptRoot 'checkpoint.c') -Raw)
$tag = if ($Fixed) {
    'fixed'
}
else {
    'original'
}
$unit = Join-Path $testRoot "$tag-unit.c"
[IO.File]::WriteAllText($unit, $body)
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_STANDARD = $null
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (Join-Path $snapshot 'base/include')
    $options = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    foreach ($g in 1, 2) {
        $obj = Join-Path $testRoot "$tag$g.c1o"
        $exe = Join-Path $testRoot "$tag$g.exe"
        Compile (Join-Path $build "host/c1-stage$g.exe") $unit $obj
        & (Join-Path $build "link-stage$g.exe") $exe __c1_start $obj (Join-Path $build "host/runtime$g.c1o") ($options -split ' ')
        if ($LASTEXITCODE) {
            throw 'Link failed'
        }
        $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Run timeout'
        }
        $p.WaitForExit()
        $expected = if ($Fixed) {
            0
        }
        else {
            1
        }
        if ($p.ExitCode -ne $expected) {
            throw "Expected $expected got $($p.ExitCode)"
        }
        "PASS $tag generation $g checkpoint test exit $expected"
    }
    foreach ($ext in 'c1o', 'exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $testRoot "$($tag)1.$ext"))
        $b = [IO.File]::ReadAllBytes((Join-Path $testRoot "$($tag)2.$ext"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw 'Bytes differ'
            }
        }
    }
    "PASS $tag checkpoint two artifact pairs equal"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
