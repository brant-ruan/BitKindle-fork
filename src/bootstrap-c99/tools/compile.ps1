
. (Join-Path $PSScriptRoot '../base/ps51-compat.ps1')
$ErrorActionPreference = 'Stop'
function Compile([string]$Tool, [string]$Source, [string]$Output) {
    $watch = [Diagnostics.Stopwatch]::StartNew()
    if ([IO.Path]::GetFileName($Tool) -match '^c1-(stage[0-9]+|bridge|capacity)\.exe$') {
        # Only buffer the original bytes flowing through standard streams.
        # C0/A1 need regular-file EOF semantics and retain the path below.
        $info = [Diagnostics.ProcessStartInfo]::new($Tool)
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $info.RedirectStandardInput = $true
        $info.RedirectStandardOutput = $true
        $process = [Diagnostics.Process]::new()
        $process.StartInfo = $info
        $sourceStream = $null
        $outputStream = $null
        $started = $false
        try {
            $sourceStream = [IO.File]::OpenRead($Source)
            $outputStream = [IO.File]::Create($Output)
            [void]$process.Start()
            $started = $true
            $outTask = $process.StandardOutput.BaseStream.CopyToAsync($outputStream)
            $inTask = $sourceStream.CopyToAsync($process.StandardInput.BaseStream)
            if (-not $inTask.Wait(120000)) {
                throw "Input timeout: $Tool"
            }
            $process.StandardInput.Close()
            if (-not $process.WaitForExit(120000)) {
                throw "Compile timeout: $Tool"
            }
            if (-not $outTask.Wait(120000)) {
                throw "Output timeout: $Tool"
            }
            if ($process.ExitCode -ne 0) {
                throw "Compile failed ($($process.ExitCode)): $Tool"
            }
        }
        finally {
            if ($started -and -not $process.HasExited) {
                $process.Kill()
            }
            if ($sourceStream) {
                $sourceStream.Dispose()
            }
            if ($outputStream) {
                $outputStream.Dispose()
            }
            $process.Dispose()
        }
    }
    else {
        $p = Start-Process -FilePath $Tool -WindowStyle Hidden -PassThru -RedirectStandardInput $Source -RedirectStandardOutput $Output
        [void]$p.Handle
        if (-not $p.WaitForExit(120000)) {
            $p.Kill()
            throw "Compile timeout: $Tool" 
        }
        $p.WaitForExit()
        if ($p.ExitCode -ne 0) {
            throw "Compile failed ($($p.ExitCode)): $Tool" 
        }
    }
    "Built $([IO.Path]::GetFileName($Output)) in $([C1Math]::Round($watch.Elapsed.TotalSeconds,2)) s"
}

