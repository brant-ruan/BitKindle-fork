param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$ToolDirectory)
$Original = $false
$testRoot = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/character-sign'
New-Item -ItemType Directory -Force $testRoot | Out-Null
$toolBuild = if ($ToolDirectory) {
    [IO.Path]::GetFullPath($ToolDirectory)
}
else {
    [IO.Path]::GetFullPath($BuildDirectory)
}
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Get-Content "$snapshot/base/tests/test-c0.ps1" -Raw
$prefix = $source.Substring(0, $source.IndexOf("Check 'constant'")).Replace("'^C1 error: '", "'(?m)^C1 error: '")
$cases = Get-Content (Join-Path $PSScriptRoot 'character-sign/cases.json') -Raw | ConvertFrom-Json
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
try {
    $env:C1_OUTPUT = $null
    $env:C1_INCLUDE = $null
    $env:C1_RUNTIME = $null
    $tag = if ($Original) {
        'original'
    }
    else {
        'fixed'
    }
    $commands = foreach ($c in $cases) {
        $result = if ($Original) {
            [int]$c.originalResult
        }
        else {
            0
        }
        $reject = $Original -and $c.originalReject
        "Check '$($c.name)' '" + $c.source.Replace("'", "''") + "' $result " + $(if ($reject) {
                '$true'
            }
            else {
                '$false'
            })
    }
    $selected = $prefix + ($commands -join "`n") + "`n" + '"PASS $script:passed character sign cases"'
    [IO.File]::WriteAllText((Join-Path $testRoot "$tag-selected.ps1"), $selected)
    foreach ($mode in 'c89', 'c99') {
        $env:C1_STANDARD = $mode
        foreach ($g in 1, 2) {
            $compiler = Join-Path $toolBuild "host/c1-stage$g.exe"
            & ([scriptblock]::Create($selected)) -Compiler $compiler -Assembler "$toolBuild/link-stage2.exe" -CompilerKind C1 -ScratchDirectory (Join-Path $testRoot "$tag-$mode-$g")
            if (!$?) {
                throw 'Character sign suite failed'
            }
        }
    }
    $pairs = 0
    foreach ($mode in 'c89', 'c99') {
        foreach ($c in $cases) {
            if ($Original -and $c.originalReject) {
                continue
            }
            $a = [IO.File]::ReadAllBytes((Join-Path $testRoot "$tag-$mode-1/$($c.name).exe"))
            $b = [IO.File]::ReadAllBytes((Join-Path $testRoot "$tag-$mode-2/$($c.name).exe"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Unequal lengths'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Different bytes'
                }
            }
            $pairs++
        }
    }
    "PASS character sign suite: $pairs equal executable pairs"
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
