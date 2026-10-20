param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$root = Join-Path ([IO.Path]::GetFullPath($BuildDirectory)) 'namespace/vla'
$log = [IO.File]::ReadAllText((Join-Path $BuildDirectory 'VLA.txt'))
$data = Get-Content (Join-Path $PSScriptRoot 'vla-cases.json') -Raw | ConvertFrom-Json
$pairs = 0
$rejected = 0
foreach ($kind in 'positive', 'negative') {
    foreach ($entry in $data.$kind.PSObject.Properties) {
        $name = if ($kind -eq 'negative') {
            'reject-' + $entry.Name
        }
        else {
            $entry.Name
        }
        if ([regex]::Matches($log, '(?m)^PASS C1 ' + [regex]::Escape($name) + '\r?$').Count -ne 2) {
            throw "Missing execution gates: $name"
        }
        foreach ($g in 1, 2) {
            $p = "$root/stage$g/$name"
            if ([IO.File]::ReadAllText("$p.c") -cne $entry.Value) {
                throw "Fixture mismatch: $name"
            }
            $bytes = [IO.File]::ReadAllBytes("$p.exe")
            $err = [IO.File]::ReadAllText("$p.err")
            if ($kind -eq 'negative') {
                if ($bytes.Length -or $err -notmatch '(?m)^C1 error: ') {
                    throw "Invalid rejection: $name"
                }
                $rejected++
            }
            else {
                if ($bytes.Length -lt 1024 -or $bytes[0] -ne 77 -or $bytes[1] -ne 90 -or $err.Length) {
                    throw "Invalid accepted PE: $name"
                }
                foreach ($suffix in 'stdout', 'stderr') {
                    if ((Get-Item "$p.$suffix").Length) {
                        throw "Unexpected runtime output: $name"
                    }
                }
            }
        }
        if ($kind -eq 'positive') {
            $a = [IO.File]::ReadAllBytes("$root/stage1/$name.exe")
            $b = [IO.File]::ReadAllBytes("$root/stage2/$name.exe")
            if ($a.Length -ne $b.Length) {
                throw 'PE length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'PE byte mismatch'
                }
            }
            $pairs++
        }
    }
}
$marker = "PASS $($pairs*2) runtime VLA executions, $rejected constraint rejections and $pairs equal pairs"
if (!$log.TrimEnd().EndsWith($marker)) {
    throw 'Missing VLA final gate'
}
"PASS VLA source fixtures, $($pairs*2) execution gates, $rejected rejections and $pairs equal PE pairs"
