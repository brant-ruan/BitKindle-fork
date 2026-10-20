param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../../tools/compile.ps1')
$saved = $env:C1_OUTPUT
try {
    foreach ($g in 1, 2) {
        $root = Join-Path $BuildDirectory "features/compound/objects$g"
        New-Item -ItemType Directory -Force $root | Out-Null
        $compiler = Join-Path $BuildDirectory "host/c1-stage$g.exe"
        $env:C1_OUTPUT = 'object'
        foreach ($name in 'a', 'b', 'external') {
            Compile $compiler (Join-Path $PSScriptRoot "link-tests/$name.c") (Join-Path $root "$name.c1o")
        }
        $env:C1_OUTPUT = $null
        $exe = Join-Path $root 'linked.exe'
        $linkArguments = '"' + $exe + '" main "' + $root + '/a.c1o" "' + $root + '/b.c1o" "' + $root + '/external.c1o"'
        $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList $linkArguments -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Link timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw 'Link failed'
        }
        $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
        [void]$p.Handle
        if (!$p.WaitForExit(30000)) {
            $p.Kill()
            throw 'Execution timeout'
        }
        $p.WaitForExit()
        if ($p.ExitCode) {
            throw "Compound relocation failure $($p.ExitCode)"
        }
        "PASS generation $g file-scope compound addresses across three translation units"
    }
    foreach ($name in 'a.c1o', 'b.c1o', 'external.c1o', 'linked.exe') {
        $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "features/compound/objects1/$name"))
        $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "features/compound/objects2/$name"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Bytes differ $name"
            }
        }
        "PASS identical $name"
    }
}
finally {
    $env:C1_OUTPUT = $saved
}

