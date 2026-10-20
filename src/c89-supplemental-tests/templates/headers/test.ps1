param([Parameter(Mandatory = $true)][string]$Snapshot, [Parameter(Mandatory = $true)][string]$BuildDirectory, [switch]$ObjectMode)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath($Snapshot)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$headers = 'assert ctype errno float limits locale math setjmp signal stdarg stddef stdio stdlib string time'.Split(' ')
$orders = [ordered]@{}
foreach ($a in $headers) {
    foreach ($b in $headers) {
        if ($a -ne $b) {
            $orders["pair-$a-$b"] = @($a, $b)
        }
    }
}
foreach ($direction in 1, -1) {
    for ($start = 0; $start -lt $headers.Count; $start++) {
        $order = @()
        for ($i = 0; $i -lt $headers.Count; $i++) {
            $order += $headers[($start + $direction * $i + $headers.Count) % $headers.Count]
        }
        $orders["all-$direction-$start"] = $order
    }
}
if ($orders.Count -ne 240) {
    throw 'Wrong header order inventory'
}
$saved = @{}
foreach ($name in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = $null
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { Join-Path $snapshot $_ }) -join ';'
    foreach ($g in 1, 2) {
        $dir = Join-Path $PSScriptRoot "generation$g"
        New-Item -ItemType Directory -Force $dir | Out-Null
        $compiler = Join-Path $build "host/c1-stage$g.exe"
        foreach ($name in $orders.Keys) {
            $source = Join-Path $dir "$name.c"
            $exe = Join-Path $dir "$name.exe"
            $err = Join-Path $dir "$name.stderr"
            $text = (($orders[$name] | ForEach-Object { "#include <$_.h>`n#include <$_.h>" }) -join "`n") + "`nint main(void){return 0;}`n"
            [IO.File]::WriteAllText($source, $text, [Text.Encoding]::ASCII)
            $p = Start-Process -FilePath $compiler -WindowStyle Hidden -PassThru -RedirectStandardInput $source -RedirectStandardOutput $exe -RedirectStandardError $err
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Header compile timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Header order compile failed: $g $name"
            }
            $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Header executable timeout'
            }
            $p.WaitForExit()
            if ($p.ExitCode) {
                throw "Header client failed: $g $name"
            }
            "PASS generation $g headers $name"
        }
    }
    foreach ($name in $orders.Keys) {
        $a = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "generation1/$name.exe"))
        $b = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot "generation2/$name.exe"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Header artifact length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Header artifact mismatch: $name"
            }
        }
    }
    'PASS 480 C89 header order executions and 240 nonempty equal executable pairs'
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
