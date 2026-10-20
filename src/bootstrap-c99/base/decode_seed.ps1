#requires -Version 5.1
param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Output
)
$ErrorActionPreference = 'Stop'
$sourcePath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Source)
$outputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Output)
if ([string]::Equals($sourcePath, $outputPath, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Source and output must differ'
}
# Only decode literal byte pairs. No assembler, PE layout or relocation logic.
$literal = [IO.File]::ReadAllText($sourcePath)
$literal = [regex]::Replace($literal, '(?m)#[^\r\n]*', '')
if (![regex]::IsMatch($literal, '\A(?:[ \t\r\n\v\f]*[0-9a-fA-F]{2})*[ \t\r\n\v\f]*\z')) {
    throw 'Invalid hexadecimal text: expected complete byte pairs and ASCII whitespace'
}
$hex = [regex]::Replace($literal, '[ \t\r\n\v\f]', '')
$bytes = New-Object byte[] ($hex.Length / 2)
for ($i = 0; $i -lt $bytes.Length; $i++) {
    $bytes[$i] = [Convert]::ToByte($hex.Substring(2 * $i, 2), 16)
}
# Validate the entire input before touching the destination. Write raw bytes.
[IO.File]::WriteAllBytes($outputPath, $bytes)
