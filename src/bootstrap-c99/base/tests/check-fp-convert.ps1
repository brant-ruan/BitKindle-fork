param(
    [Parameter(Mandatory = $true)][string]$Compiler,
    [Parameter(Mandatory = $true)][string]$ScratchDirectory,
    [switch]$BootstrapSource
)
. (Join-Path $PSScriptRoot '../ps51-compat.ps1')

$ErrorActionPreference = 'Stop'
$Compiler = [IO.Path]::GetFullPath($Compiler)
$ScratchDirectory = [IO.Path]::GetFullPath($ScratchDirectory)
[void][IO.Directory]::CreateDirectory($ScratchDirectory)
function Invoke-Raw([string]$Tool, [string]$InputPath, [string]$OutputPath, [string]$ErrorPath) {
    $info = [Diagnostics.ProcessStartInfo]::new($Tool)
    $info.WorkingDirectory = $PSScriptRoot
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $p = [Diagnostics.Process]::new()
    $p.StartInfo = $info
    $inputStream = $null
    $outputStream = $null
    $errorStream = $null
    $started = $false
    try {
        $inputStream = [IO.File]::OpenRead($InputPath)
        $outputStream = [IO.File]::Create($OutputPath)
        $errorStream = [IO.File]::Create($ErrorPath)
        [void]$p.Start()
        $started = $true
        $outTask = $p.StandardOutput.BaseStream.CopyToAsync($outputStream)
        $errTask = $p.StandardError.BaseStream.CopyToAsync($errorStream)
        $inTask = $inputStream.CopyToAsync($p.StandardInput.BaseStream)
        if (!$inTask.Wait(60000)) {
            throw 'Input timeout'
        }
        $p.StandardInput.Close()
        if (!$p.WaitForExit(60000)) {
            throw 'Process timeout'
        }
        if (![Threading.Tasks.Task]::WaitAll([Threading.Tasks.Task[]]@($outTask, $errTask), 60000)) {
            throw 'Output timeout'
        }
        if ($p.ExitCode) {
            throw "Process returned $($p.ExitCode): $Tool"
        }
    }
    finally {
        if ($started -and !$p.HasExited) {
            $p.Kill()
        }
        if ($inputStream) {
            $inputStream.Dispose()
        }
        if ($outputStream) {
            $outputStream.Dispose()
        }
        if ($errorStream) {
            $errorStream.Dispose()
        }
        $p.Dispose()
    }
}
$exe = Join-Path $ScratchDirectory 'fp-convert.exe'
$source = Join-Path $PSScriptRoot 'fp-convert-driver.c'
if ($BootstrapSource) {
    # Source-text editing only: the frozen compiler predates preprocessing.
    $driver = [IO.File]::ReadAllText($source).Replace('#include "../c1/fp-convert.c"', '')
    $source = Join-Path $ScratchDirectory 'fp-convert-flat.c'
    [IO.File]::WriteAllText($source, [IO.File]::ReadAllText((Join-Path $PSScriptRoot '../c1/fp-convert.c')) + [char]10 + $driver, [Text.Encoding]::ASCII)
}
# Only the custom compiler generates executable bytes. Streams remain unchanged.
Invoke-Raw $Compiler $source $exe (Join-Path $ScratchDirectory 'compile.err')
$vectors = [Collections.Generic.List[object]]::new()
function Vector([string]$Name, [string]$Token, [string]$Bits = '', [int]$Type = 14, [int]$Status = 0, [int]$Inexact = -1, [int]$Underflow = -1) {
    $vectors.Add([pscustomobject]@{Name = $Name; Token = $Token; Bits = $Bits; Type = $Type; Status = $Status; Inexact = $Inexact; Underflow = $Underflow })
}
function Exact([string]$Name, [string]$Token, [string]$Bits, [int]$Type = 14) {
    Vector $Name $Token $Bits $Type 0 0 0 
}
Exact 'zero' '0.0' '0000000000000000'
Exact 'zero-huge-exp' '0e999999999999999999999999' '0000000000000000'
Exact 'zero-tiny-exp' '0e-999999999999999999999999f' '0000000000000000' 13
Exact 'one' '1.' '3FF0000000000000'
Exact 'half' '.5' '3FE0000000000000'
Exact 'float-one' '1e0F' '000000003F800000' 13
Exact 'long-double-one' '1.0L' '3FF0000000000000' 15
Exact 'hex-point-leading' '0x.8p+1' '3FF0000000000000'
Exact 'hex-point-trailing' '0X1.P0' '3FF0000000000000'
Exact 'hex-e-digit' '0x1e0p0' '407E000000000000'
Exact 'double-min-normal' '0x1p-1022' '0010000000000000'
Exact 'double-min-subnormal' '0x1p-1074' '0000000000000001'
Exact 'double-max-subnormal' '0xFFFFFFFFFFFFFp-1074' '000FFFFFFFFFFFFF'
Exact 'double-max-finite' '0x1FFFFFFFFFFFFFp971' '7FEFFFFFFFFFFFFF'
Exact 'float-min-normal' '0x1p-126f' '0000000000800000' 13
Exact 'float-min-subnormal' '0x1p-149f' '0000000000000001' 13
Exact 'float-max-subnormal' '0x7FFFFFp-149f' '00000000007FFFFF' 13
Exact 'float-max-finite' '0xFFFFFFp104f' '000000007F7FFFFF' 13
Vector 'decimal-tenth' '0.1' '3FB999999999999A' 14 0 1 0
Vector 'decimal-tenth-float' '0.1f' '000000003DCCCCCD' 13 0 1 0
Vector 'float-halfway-even-low' '1.000000059604644775390625f' '000000003F800000' 13 0 1 0
Vector 'float-halfway-even-high' '1.000000178813934326171875f' '000000003F800002' 13 0 1 0
Vector 'float-avoid-double-round-up' '1.00000005960464477539062500000000000000000001f' '000000003F800001' 13 0 1 0
Vector 'float-avoid-double-round-down' '1.00000017881393432617187499999999999999999999f' '000000003F800001' 13 0 1 0
Vector 'float-long-tail-above-midpoint' ('1.000000059604644775390625' + ('0' * 8000) + '1f') '000000003F800001' 13 0 1 0
Vector 'double-long-tail-above-midpoint' ('1.00000000000000011102230246251565404236316680908203125' + ('0' * 8000) + '1') '3FF0000000000001' 14 0 1 0
Vector 'double-halfway-even-low' '0x20000000000001p-53' '3FF0000000000000' 14 0 1 0
Vector 'double-halfway-even-high' '0x20000000000003p-53' '3FF0000000000002' 14 0 1 0
Vector 'float-half-min-subnormal' '0x1p-150f' '0000000000000000' 13 0 1 1
Vector 'double-half-min-subnormal' '0x1p-1075' '0000000000000000' 14 0 1 1
Vector 'float-round-to-normal' '0xFFFFFFp-150f' '0000000000800000' 13 0 1 0
Vector 'double-round-to-normal' '0x1FFFFFFFFFFFFFp-1075' '0010000000000000' 14 0 1 0
Vector 'float-overflow-midpoint' '0x1FFFFFFp103f' '' 13 2
Vector 'double-overflow-midpoint' '0x3FFFFFFFFFFFFFp970' '' 14 2
Vector 'huge-decimal' '1e99999999999999999999' '' 14 2
Vector 'tiny-decimal' '1e-99999999999999999999' '0000000000000000' 14 0 1 1
Vector 'huge-hex' '0x1p99999999999999999999f' '' 13 2
Vector 'tiny-hex' '0x1p-99999999999999999999f' '0000000000000000' 13 0 1 1
foreach ($bad in @('', '.', '1', '1f', '0x1', '0x1.0', '0x.p1', '1e', '1e+', '1e-', '1.e1ff', '1.0U', '0x1p0u', '.e0', '+1.0', '-1.0', ' 1.0', '1.0 ', '1..0', '1.0e1e2', '0x1p0p0', '0xg1p0', '0x1p+', '0X1P-', '1.0d', 'nan', 'inf')) {
    Vector ('invalid-' + $vectors.Count) $bad '' -1 1
}
# Exact decimal test data derived from integer powers, not production conversion code.
function DecimalScaled([System.Numerics.BigInteger]$Coefficient, [int]$Scale) {
    $digits = $Coefficient.ToString([Globalization.CultureInfo]::InvariantCulture).PadLeft($Scale + 1, '0')
    if (!$Scale) {
        return $digits + '.0'
    }
    return $digits.Insert($digits.Length - $Scale, '.')
}
foreach ($pair in @(@(13, 150), @(14, 1075))) {
    $type = $pair[0]
    $power = $pair[1]
    $suffix = if ($type -eq 13) {
        'f'
    }
    else {
        ''
    }
    $coef = [System.Numerics.BigInteger]::Pow(5, $power)
    Vector "decimal-half-$type" ((DecimalScaled $coef $power) + $suffix) '0000000000000000' $type 0 1 1
    Vector "decimal-below-half-$type" ((DecimalScaled ($coef - 1) $power) + $suffix) '0000000000000000' $type 0 1 1
    Vector "decimal-above-half-$type" ((DecimalScaled ($coef + 1) $power) + $suffix) '0000000000000001' $type 0 1 1
}
foreach ($pair in @(@(13, 128, 103, '000000007F7FFFFF'), @(14, 1024, 970, '7FEFFFFFFFFFFFFF'))) {
    $type = $pair[0]
    $suffix = if ($type -eq 13) {
        'f'
    }
    else {
        ''
    }
    $mid = [System.Numerics.BigInteger]::Pow(2, $pair[1]) - [System.Numerics.BigInteger]::Pow(2, $pair[2])
    Vector "decimal-overflow-mid-$type" ((DecimalScaled $mid 0) + $suffix) '' $type 2
    Vector "decimal-below-overflow-$type" ((DecimalScaled ($mid - 1) 0) + $suffix) $pair[3] $type 0 1 0
}
Exact 'long-decimal-one' ('1' + ('0' * 8191) + '.e-8191') '3FF0000000000000'
Exact 'long-hex-one' ('0x1' + ('0' * 8191) + 'p-32764') '3FF0000000000000'
Vector 'long-hex-half-subnormal' ('0x8' + ('0' * 8191) + 'p-33842') '0000000000000000' 14 0 1 1
Vector 'long-hex-above-half-subnormal' ('0x8' + ('0' * 8190) + '1p-33842') '0000000000000001' 14 0 1 1
Vector 'long-hex-half-subnormal-float' ('0x8' + ('0' * 8191) + 'p-32917f') '0000000000000000' 13 0 1 1
Vector 'long-hex-above-half-subnormal-float' ('0x8' + ('0' * 8190) + '1p-32917f') '0000000000000001' 13 0 1 1
Vector 'long-hex-round-to-normal' ('0x' + ('F' * 8192) + 'p-33790') '0010000000000000' 14 0 1 0
Vector 'long-hex-overflow' ('0x' + ('F' * 8192) + 'p-31744') '' 14 2
Vector 'decimal-resource-limit' ('1' + ('0' * 8192) + '.e-8192') '' 14 3
Vector 'hex-resource-limit' ('0x1' + ('0' * 8192) + 'p-32768') '' 14 3
Exact 'many-leading-zeros' (('0' * 10000) + '1.0') '3FF0000000000000'
Exact 'maximum-token-length' (('0' * 1048573) + '1.0') '3FF0000000000000'
Vector 'over-maximum-token-length' (('0' * 1048574) + '1.0') '' 14 3
# Independent test-only exact rational parsing oracle. No executable or object bytes are created by .NET.
$rng = [Random]::new(730149)
$culture = [Globalization.CultureInfo]::InvariantCulture
foreach ($type in 13, 14, 15) {
    for ($i = 0; $i -lt 500; $i++) {
        $exp = if ($type -eq 13) {
            $rng.Next(-60, 51)
        }
        else {
            $rng.Next(-340, 321)
        }
        $text = $rng.Next(1, 2147483647).ToString($culture) + '.' + $rng.Next(0, 2147483647).ToString($culture) + 'e' + $exp.ToString($culture)
        if ($type -eq 13) {
            $number = [C1MathF]::Parse($text, [Globalization.NumberStyles]::Float, $culture)
            $infinite = [single]::IsInfinity($number)
            $bits = ([uint32][C1Bits]::SingleToInt32Bits($number)).ToString('X16')
        }
        else {
            $number = [C1Math]::Parse($text, [Globalization.NumberStyles]::Float, $culture)
            $infinite = [double]::IsInfinity($number)
            $bits = ([uint64][BitConverter]::DoubleToInt64Bits($number)).ToString('X16')
        }
        $suffix = if ($type -eq 13) {
            'f'
        }
        elseif ($type -eq 15) {
            'L'
        }
        else {
            ''
        }
        if ($infinite) {
            Vector "oracle-$type-$i" ($text + $suffix) '' $type 2
        }
        else {
            Vector "oracle-$type-$i" ($text + $suffix) $bits $type
        }
    }
}
# Inverse construction of exact representable values, including subnormals.
foreach ($type in 13, 14) {
    for ($i = 0; $i -lt 200; $i++) {
        if ($type -eq 13) {
            $p = 23
            $bias = 127
            $exp = $rng.Next(1, 255)
            $fraction = [uint64]$rng.Next(1, 8388608)
            $suffix = 'f'
        }
        else {
            $p = 52
            $bias = 1023
            $exp = $rng.Next(1, 2047)
            $fraction = ([uint64]$rng.Next(0, 67108864) -shl 26)-bor [uint64]$rng.Next(1, 67108864)
            $suffix = ''
        }
        $mantissa = ([uint64]1 -shl $p)-bor $fraction
        $bits = (([uint64]$exp -shl $p)-bor $fraction).ToString('X16')
        Exact "hex-normal-$type-$i" ('0x' + $mantissa.ToString('X') + 'p' + ($exp - $bias - $p) + $suffix) $bits $type
        Exact "hex-subnormal-$type-$i" ('0x' + $fraction.ToString('X') + 'p' + (1 - $bias - $p) + $suffix) ($fraction.ToString('X16')) $type
    }
}
$inputPath = Join-Path $ScratchDirectory 'vectors.txt'
$outputPath = Join-Path $ScratchDirectory 'results.txt'
[IO.File]::WriteAllText($inputPath, ([string]::Join([char]10, ($vectors | ForEach-Object { $_.Token }))) + [char]10, [Text.Encoding]::ASCII)
Invoke-Raw $exe $inputPath $outputPath (Join-Path $ScratchDirectory 'runtime.err')
$lines = [IO.File]::ReadAllLines($outputPath)
if ($lines.Count -ne $vectors.Count) {
    throw "Expected $($vectors.Count) results, received $($lines.Count)"
}
for ($i = 0; $i -lt $vectors.Count; $i++) {
    $v = $vectors[$i]
    $fields = $lines[$i].Split(',')
    if ($fields.Count -ne 5) {
        throw "Invalid protocol at $i"
    }
    if ([int]$fields[0] -ne $v.Status -or ($v.Type -ge 0 -and [int]$fields[1] -ne $v.Type) -or
        ($v.Bits -and $fields[2] -cne $v.Bits) -or ($v.Inexact -ge 0 -and [int]$fields[3] -ne $v.Inexact) -or
        ($v.Underflow -ge 0 -and [int]$fields[4] -ne $v.Underflow)) {
        throw ("Mismatch " + $v.Name + ": " + $lines[$i] + "; expected " + ($v | ConvertTo-Json -Compress))
    }
}
"PASS: $($vectors.Count) floating-token conversion vectors"
"Compiler SHA256: $((Get-FileHash $Compiler).Hash)"
"Converter source SHA256: $((Get-FileHash (Join-Path $PSScriptRoot '../c1/fp-convert.c')).Hash)"
"Driver source SHA256: $((Get-FileHash (Join-Path $PSScriptRoot 'fp-convert-driver.c')).Hash)"
"Custom-generated driver SHA256: $((Get-FileHash $exe).Hash)"
