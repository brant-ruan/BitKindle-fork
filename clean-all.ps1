#requires -Version 5.1
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param()
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\', '/')
$prefix = $root + [IO.Path]::DirectorySeparatorChar
# Only default output locations owned by the published build/example scripts.
$targets = @(
    @{Path = 'src/bootstrap-c99/build-from-seed'; Directory = $true },
    @{Path = 'work/release-run'; Directory = $true },
    @{Path = 'work/walkthrough'; Directory = $true },
    @{Path = 'src/examples/minicurl/build'; Directory = $true },
    @{Path = 'src/examples/minicurl/minicurl.exe'; Directory = $false },
    @{Path = 'src/examples/mario/build'; Directory = $true }
)
function Assert-NoLink([IO.FileSystemInfo]$Item) {
    if (($Item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing reparse point (junction/symlink): $($Item.FullName)"
    }
}
function Inspect-Target([string]$Path, [bool]$Directory) {
    # Check existing ancestors before enumerating; never traverse a junction.
    $relative = $Path.Substring($prefix.Length)
    $current = $root
    Assert-NoLink (Get-Item -LiteralPath $root -Force)
    foreach ($part in ($relative -split '[\\/]')) {
        $current = Join-Path $current $part
        $item = Get-Item -LiteralPath $current -Force -ErrorAction SilentlyContinue
        if ($null -eq $item) {
            return $null
        }
        Assert-NoLink $item
    }
    if ([bool]$item.PSIsContainer -ne $Directory) {
        throw "Unexpected target type: $Path"
    }
    $bytes = [long]0
    $files = 0
    $pending = New-Object 'System.Collections.Generic.Stack[System.IO.FileSystemInfo]'
    $pending.Push($item)
    while ($pending.Count) {
        $entry = $pending.Pop()
        Assert-NoLink $entry
        if ($entry.PSIsContainer -or $entry -is [IO.DirectoryInfo]) {
            foreach ($child in Get-ChildItem -LiteralPath $entry.FullName -Force) {
                $pending.Push($child)
            }
        }
        else {
            $bytes += $entry.Length
            $files++
        }
    }
    [pscustomobject]@{Path = $Path
        Bytes = $bytes
        Files = $files
    }
}
# Preflight all deletion roots before any mutation.
# Keep frozen toolchain/test source paths protected without checking file hashes.
$protected = @()
foreach ($name in 'bootstrap-c99', 'c89-supplemental-tests', 'c89-implementation-tests', 'language-regression-tests') {
    $package = Join-Path $root ('src/' + $name)
    foreach ($entry in (Get-Content -LiteralPath (Join-Path $package 'SOURCES.json') -Raw | ConvertFrom-Json)) {
        if ([string]::IsNullOrWhiteSpace($entry.path)) {
            throw 'Empty source path'
        }
        $file = [IO.Path]::GetFullPath((Join-Path $package $entry.path))
        if (!$file.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Source path escapes release root'
        }
        $protected += $file
    }
}
$plan = @()
foreach ($target in $targets) {
    $path = [IO.Path]::GetFullPath((Join-Path $root $target.Path))
    if (!$path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Target escapes release root: $path"
    }
    foreach ($file in $protected) {
        if ($file.Equals($path, [StringComparison]::OrdinalIgnoreCase) -or $file.StartsWith($path + '\', [StringComparison]::OrdinalIgnoreCase)) {
            throw "Target overlaps published input: $file"
        }
    }
    $entry = Inspect-Target $path $target.Directory
    if ($null -ne $entry) {
        $plan += [pscustomobject]@{Path = $path
            Directory = $target.Directory
            Bytes = $entry.Bytes
            Files = $entry.Files
        }
    }
}
$removed = 0
foreach ($entry in $plan) {
    $relative = $entry.Path.Substring($prefix.Length)
    Write-Host ("{0}: {1} files, {2:N0} bytes" -f $relative, $entry.Files, $entry.Bytes)
    if ($PSCmdlet.ShouldProcess($entry.Path, 'Remove generated build/test output')) {
        # Recheck immediately before deletion; do not run alongside builds.
        $null = Inspect-Target $entry.Path $entry.Directory
        Remove-Item -LiteralPath $entry.Path -Recurse -Force -ErrorAction Stop
        $removed++
    }
}
if ($WhatIfPreference) {
    Write-Host "Preview complete: $($plan.Count) output locations; no files removed."
}
elseif ($plan.Count -eq 0) {
    Write-Host 'Already clean: no default build/test outputs found.'
}
else {
    Write-Host "Clean complete: removed $removed of $($plan.Count) output locations."
}
