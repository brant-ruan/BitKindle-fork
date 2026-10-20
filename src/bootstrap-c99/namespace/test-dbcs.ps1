param([Parameter(Mandatory = $true)][string]$BuildDirectory, [string]$CompilerDirectory, [string]$ToolDirectory)
$ErrorActionPreference = 'Stop'
$snapshot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
if (!$ToolDirectory) {
    $ToolDirectory = $build
}
$toolBuild = [IO.Path]::GetFullPath($ToolDirectory)
$testRoot = Join-Path $build 'namespace/dbcs'
New-Item -ItemType Directory -Force $testRoot | Out-Null
. (Join-Path $snapshot 'tools/compile.ps1')
$saved = @{}
foreach ($n in 'C1_STANDARD', 'C1_OUTPUT', 'C1_INCLUDE', 'C1_RUNTIME') {
    $saved[$n] = [Environment]::GetEnvironmentVariable($n)
}
function Run([string]$file) {
    $p = Start-Process -FilePath $file -WindowStyle Hidden -PassThru
    [void]$p.Handle
    if (!$p.WaitForExit(30000)) {
        $p.Kill()
        throw 'Run timeout'
    }
    $p.WaitForExit()
    return $p.ExitCode
}
try {
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null
    $env:C1_INCLUDE = (Join-Path $snapshot 'include') + ';' + (Join-Path $snapshot 'base/include')
    $imports = ((Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json) | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
    [IO.File]::WriteAllText((Join-Path $testRoot 'acp.c'), 'unsigned int GetACP(void);int main(void){return (int)GetACP();}')
    foreach ($g in 1, 2) {
        $compiler = if ($CompilerDirectory) {
            Join-Path $CompilerDirectory "c1-stage$g.exe"
        }
        else {
            Join-Path $toolBuild "host/c1-stage$g.exe"
        }
        $linker = Join-Path $toolBuild "link-stage$g.exe"
        $runtime = Join-Path $toolBuild "namespace/runtime-policy$g/runtime.c1o"
        $obj = Join-Path $testRoot "acp$g.c1o"
        $exe = Join-Path $testRoot "acp$g.exe"
        Compile $compiler (Join-Path $testRoot 'acp.c') $obj
        & $linker $exe __c1_start $obj $runtime --import GetACP KERNEL32.dll GetACP ($imports -split ' ')
        if ($LASTEXITCODE) {
            throw 'ACP link failed'
        }
        $cp = Run $exe
        if ($g -eq 1) {
            $firstCp = $cp
        }
        elseif ($cp -ne $firstCp) {
            throw 'ACP changed'
        }
        if ($PSVersionTable.PSEdition -eq 'Core') {
            [Text.Encoding]::RegisterProvider([Text.CodePagesEncodingProvider]::Instance) 
        }
        $encoding = [Text.Encoding]::GetEncoding($cp, [Text.EncoderFallback]::ExceptionFallback, [Text.DecoderFallback]::ExceptionFallback)
        $name = $null
        foreach ($point in 0x4e00..0x9fff) {
            try {
                $candidate = [string][char]$point
                $bytes = $encoding.GetBytes($candidate)
                if ($bytes.Length -eq 2 -and $bytes[1] -eq 92 -and $encoding.GetString($bytes) -ceq $candidate) {
                    $name = $candidate
                    break
                }
            }
            catch {
            }
        }
        $hasDbcs = $null -ne $name
        if (!$name) {
            foreach ($candidate in @([string][char]0x00e9, [string][char]0x4e2d, [string][char]0x0416, [string][char]0x3042, [string][char]0x03a9)) {
                try {
                    $bytes = $encoding.GetBytes($candidate)
                    if ($encoding.GetString($bytes) -ceq $candidate) {
                        $name = $candidate
                        break
                    }
                }
                catch {
                }
            }
        }
        if (!$name) {
            throw "No representable non-ASCII test name for ACP $cp"
        }
        $dir = Join-Path $testRoot ($name + ' headers')
        New-Item -ItemType Directory -Force $dir | Out-Null
        [IO.File]::WriteAllText((Join-Path $dir 'value.h'), "#define PATH_VALUE 37`n")
        [IO.File]::WriteAllText((Join-Path $dir ($name + '.h')), "#include `"value.h`"`n", $encoding)
        $path = (Join-Path $dir ($name + '.h')).Replace('\', '/')
        $body = '#include "' + $path + '"' + [char]10 + 'int main(void){return PATH_VALUE==37?0:1;}' + [char]10
        $source = Join-Path $testRoot 'probe.c'
        [IO.File]::WriteAllText($source, $body, $encoding)
        $obj = Join-Path $testRoot "probe$g.c1o"
        $exe = Join-Path $testRoot "probe$g.exe"
        Compile $compiler $source $obj
        & $linker $exe __c1_start $obj $runtime ($imports -split ' ')
        if ($LASTEXITCODE) {
            throw 'Header path link failed'
        }
        if ((Run $exe) -ne 0) {
            throw 'Header path execution failed'
        }
        "PASS generation $g ACP $cp non-ASCII absolute and nested-relative header paths"
        $envDir = Join-Path $testRoot ('env-' + $name)
        New-Item -ItemType Directory -Force $envDir | Out-Null
        [IO.File]::WriteAllText((Join-Path $envDir 'value.h'), "#define PATH_VALUE 41`n")
        [IO.File]::WriteAllText($source, "#include <value.h>`nint main(void){return PATH_VALUE==41?0:1;}`n")
        $previousInclude = $env:C1_INCLUDE
        $env:C1_INCLUDE = $envDir
        $obj = Join-Path $testRoot "envprobe$g.c1o"
        $exe = Join-Path $testRoot "envprobe$g.exe"
        try {
            Compile $compiler $source $obj
        }
        finally {
            $env:C1_INCLUDE = $previousInclude
        }
        & $linker $exe __c1_start $obj $runtime ($imports -split ' ')
        if ($LASTEXITCODE) {
            throw 'Environment path link failed'
        }
        if ((Run $exe) -ne 0) {
            throw 'Environment path execution failed'
        }
        "PASS generation $g ACP $cp C1_INCLUDE non-ASCII directory; DBCS-trail-case=$hasDbcs"
    }
    foreach ($name in 'acp', 'probe', 'envprobe') {
        foreach ($ext in 'c1o', 'exe') {
            $a = [IO.File]::ReadAllBytes((Join-Path $testRoot "$($name)1.$ext"))
            $b = [IO.File]::ReadAllBytes((Join-Path $testRoot "$($name)2.$ext"))
            if (!$a.Length -or $a.Length -ne $b.Length) {
                throw 'Artifact length mismatch'
            }
            for ($i = 0; $i -lt $a.Length; $i++) {
                if ($a[$i] -ne $b[$i]) {
                    throw 'Artifact byte mismatch'
                }
            }
        }
    }
    'PASS header path checks and six equal artifact pairs'
}
finally {
    foreach ($n in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($n, $saved[$n])
    }
}
