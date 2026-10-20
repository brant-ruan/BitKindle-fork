param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build-from-seed-retry'),
    [string]$VerificationLog = (Join-Path $PSScriptRoot 'VERIFICATION.txt')
)
$ErrorActionPreference = 'Stop'
$out = [IO.Path]::GetFullPath($PSScriptRoot)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$log = [IO.File]::ReadAllText([IO.Path]::GetFullPath($VerificationLog))
if (([regex]::Matches($log, '(?m)^PASS ')).Count -ne 10155) {
    throw 'Test total'
}
if (([regex]::Matches($log, '(?m)^4992 C1 tests passed\.\r?$')).Count -ne 2) {
    throw 'Generation counts'
}
if (-not $log.TrimEnd().EndsWith("PASS: C1 self-compilation and both generations' functional tests")) {
    throw 'Final gate'
}
function SameBytes([string]$a, [string]$b) {
    $left = [IO.File]::ReadAllBytes((Join-Path $build $a))
    $right = [IO.File]::ReadAllBytes((Join-Path $build $b))
    if ($left.Length -ne $right.Length) {
        throw 'Length mismatch'
    }
    for ($i = 0; $i -lt $left.Length; $i++) {
        if ($left[$i] -ne $right[$i]) {
            throw "Byte mismatch $a $b $i"
        }
    }
    "PASS bytes $a = $b ($($left.Length))"
}
SameBytes 'a0-seed.exe' 'a0-stage1.exe'
SameBytes 'a0-stage1.exe' 'a0-stage2.exe'
SameBytes 'a1-stage0.exe' 'a1-stage1.exe'
SameBytes 'a1-stage1.exe' 'a1-stage2.exe'
SameBytes 'c1-stage1.exe' 'c1-stage2.exe'
SameBytes 'pi-stage1.exe' 'pi-stage2.exe'
SameBytes 'two-over-pi-stage1.h' 'two-over-pi-stage2.h'
$expected = @{
    'a0-seed.exe' = 'BA0F84742B07A4A5340D0641B676EE18269943191139B570667D9984A98465E5'
    'a1-stage2.exe' = '6EB91C0749D1FF243206CBE7FAC9B9B6C0D28A17F20C1A495981C83D258348F6'
    'c0.exe' = 'DE5A2D0373FDB660EB5D8F38911D1A5EAFE5CB2D26EFEC0691D3FF7EE35193FA'
    'c1-bridge.exe' = 'F72C3B84AB77B2A0A12C07252603538D4229EBF2894D5331E316DDFD9DC4E36D'
    'c1-capacity.exe' = '9F66976BA49A793B1E1208EA5C8C003BBB972E144D62C5E3738626E60498C706'
    'pi-stage2.exe' = '13923E14493CF7BCD4C7C3D2B097A2A9EC09BA8029A7333A4A4DA25418024BBD'
    'c1-stage2.exe' = 'A31C2A4B8C4A8F816870CEA52B75109B90B9D8D40BDF2C398CA9C4A5BCB281AD'
}
foreach ($name in $expected.Keys) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $build $name) -Algorithm SHA256).Hash
    if ($hash -ne $expected[$name]) {
        throw "Hash mismatch $name"
    }
    "PASS hash $name $hash"
}
$compiler = [IO.File]::ReadAllText((Join-Path $out 'c1/compiler.c'))
foreach ($name in 'fp-convert.c', 'fp-eval.c') {
    $part = [IO.File]::ReadAllText((Join-Path $out ('c1/' + $name)))
    if (-not $compiler.Contains($part)) {
        throw "Embedded source mismatch $name"
    }
    "PASS embedded source $name"
}
if ((Get-ChildItem (Join-Path $out 'include') -File -Filter '*.h').Count -ne 19) {
    throw 'Header count'
}
'PASS hosted/time/system/wide/math integration milestone audit: 10155 checks; full C89/C99 objective remains incomplete.'
Get-ChildItem $build -File -Filter '*.exe' | Sort-Object Name | ForEach-Object {
    "INFO artifact $($_.Name) $($_.Length) $((Get-FileHash $_.FullName).Hash)"
}
