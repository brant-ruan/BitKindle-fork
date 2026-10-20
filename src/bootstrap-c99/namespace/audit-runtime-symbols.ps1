param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$SourceRoot = $PSScriptRoot
$SnapshotRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$CompilerDirectory = Join-Path $BuildDirectory 'host'
$OutputDirectory = Join-Path $BuildDirectory 'namespace'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$ErrorActionPreference = 'Stop'
$catalog = Get-Content (Join-Path $SourceRoot 'library-functions.json') -Raw | ConvertFrom-Json
$allowed = @{}
foreach ($property in $catalog.PSObject.Properties) {
    foreach ($name in $property.Value.Split(' ')) {
        $allowed[$name] = $true
    }
}
foreach ($g in 1, 2) {
    $bytes = [IO.File]::ReadAllBytes((Join-Path $OutputDirectory "runtime-policy$g/runtime.c1o"))
    if ([BitConverter]::ToUInt32($bytes, 0) -ne 0x314f3143) {
        throw 'Unexpected object format'
    }
    $count = [BitConverter]::ToUInt32($bytes, 12)
    $refs = [BitConverter]::ToUInt32($bytes, 16)
    $strings = 128 + $count * 40 + $refs * 32
    $seen = @{}
    $private = 0
    $local = 0
    for ($i = 0; $i -lt $count; $i++) {
        $record = 128 + $i * 40
        $binding = [BitConverter]::ToUInt32($bytes, $record + 4)
        $start = $strings + [BitConverter]::ToUInt32($bytes, $record)
        $end = $start
        while ($end -lt $bytes.Length -and $bytes[$end]) {
            $end++
        }
        if ($end -eq $bytes.Length) {
            throw 'Unterminated object name'
        }
        $name = [Text.Encoding]::ASCII.GetString($bytes, $start, $end - $start)
        if ($binding -eq 1) {
            $local++
            continue
        }
        if ($binding -notin 2, 3) {
            continue
        }
        if ($allowed.ContainsKey($name)) {
            $seen[$name] = $true
        }
        elseif ($name.StartsWith('_')) {
            $private++
        }
        else {
            throw "Unexpected public C89 runtime export: $name"
        }
    }
    foreach ($name in $allowed.Keys) {
        if (!$seen.ContainsKey($name)) {
            throw "Missing C89 export: $name"
        }
    }
    "PASS generation $g exports exactly $($seen.Count) catalogued C89 names plus $private reserved implementation names; $local local symbols"
}
'PASS runtime symbol audit is read-only; broader conformance and integration remain separate gates'
