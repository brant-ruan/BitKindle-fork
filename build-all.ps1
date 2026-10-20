#requires -Version 5.1
[CmdletBinding(DefaultParameterSetName = 'Default')]
param(
    [Parameter(ParameterSetName = 'All')][switch]$All,
    [Parameter(ParameterSetName = 'Full')][switch]$Full
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
Push-Location $root
$started = [DateTime]::UtcNow
try {
    if (!([Environment]::OSVersion.Platform -eq [PlatformID]::Win32NT) -or ![Environment]::Is64BitOperatingSystem) {
        throw 'Windows x64 is required' 
    }
    $snapshot = Join-Path $root 'src/bootstrap-c99'
    $build = Join-Path $snapshot 'build-from-seed'
    $logs = Join-Path $root 'work/release-run'
    if ((Test-Path -LiteralPath $build) -or (Test-Path -LiteralPath $logs)) {
        throw 'Use a fresh extraction: build-from-seed or work/release-run already exists' 
    }
    New-Item -ItemType Directory -Path $logs -Force | Out-Null
    function Step([string]$name, [scriptblock]$action) {
        Write-Host "[$([DateTime]::Now.ToString('HH:mm:ss'))] $name (log: work/release-run/$name.txt)"
        & $action *> (Join-Path $logs ($name + '.txt'))
        if (!$?) {
            throw "Failed: $name; see its log" 
        }
        Write-Host "PASS $name"
    }
    if (!$Full) {
        Step '01-fast-build' { & "$snapshot/tools/build-fast.ps1" -BuildDirectory $build }
        $compilerHash = (Get-FileHash -LiteralPath "$build/host/c1-stage2.exe").Hash
        if ($compilerHash -ne 'DB92844256433C7020D0C399E8F32D9CB9BE4954CC4063EAC0DFEAD0A4570ADD') {
            throw 'Final compiler differs from release baseline' 
        }
        if ($All) {
            Step '02-build-examples' {
                & "$snapshot/compile-program.ps1" -Source "$snapshot/examples/hello.c" -Output "$logs/hello.exe" -BuildDirectory $build -Standard c89
                if (!$?) {
                    throw 'hello build failed' 
                }
                & "$snapshot/compile-program.ps1" -Source "$snapshot/examples/args.c" -Output "$logs/args.exe" -BuildDirectory $build -Standard c89 -MainArguments
                if (!$?) {
                    throw 'args build failed' 
                }
                foreach ($example in 'minicurl', 'mario') {
                    & "$root/src/examples/$example/build.ps1" -BuildDirectory $build
                    if (!$?) {
                        throw "$example build failed" 
                    }
                }
            }
        }
        $mode = if ($All) {
            'All' 
        }
        else {
            'Fast' 
        }
        [pscustomobject]@{Status = 'BUILT'
            Mode = $mode
            ExamplesBuilt = [bool]$All
            TestsRun = $false
            StartedUtc = $started.ToString('o')
            FinishedUtc = [DateTime]::UtcNow.ToString('o')
            CompilerSHA256 = $compilerHash
            PowerShell = $PSVersionTable.PSVersion.ToString()
            Scope = 'From-seed toolchain and C89 runtime; self-compilation identity checked; regression, integration, supplemental and example tests skipped'
        } |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logs 'RESULT.json') -Encoding utf8
        Write-Host "BUILT $mode. Regression tests were NOT run. Compiler: src/bootstrap-c99/build-from-seed/host/c1-stage2.exe"
        return
    }
    Step '01-seed-build' { & "$snapshot/build.ps1" -BuildDirectory $build }
    Step '02-full-audit' { & "$snapshot/audit.ps1" -BuildDirectory $build -VerificationLog (Join-Path $logs '01-seed-build.txt') }
    foreach ($package in 'c89-supplemental-tests', 'c89-implementation-tests', 'language-regression-tests') {
        $scratch = Join-Path $logs $package
        $suite = Join-Path $root ('src/' + $package)
        Step ($package + '-run') { & "$suite/run.ps1" -Snapshot $snapshot -BuildDirectory $build -ScratchDirectory $scratch }
        Step ($package + '-audit') { & "$suite/audit.ps1" -ScratchDirectory $scratch -VerificationLog (Join-Path $logs ($package + '-run.txt')) }
    }
    Step '03-examples' {
        & "$snapshot/compile-program.ps1" -Source "$snapshot/examples/hello.c" -Output "$logs/hello.exe" -BuildDirectory $build -Standard c89
        if (!$?) {
            throw 'hello build failed' 
        }
        & "$logs/hello.exe"
        if ($LASTEXITCODE) {
            throw 'hello failed' 
        }
        & "$snapshot/compile-program.ps1" -Source "$snapshot/examples/args.c" -Output "$logs/args.exe" -BuildDirectory $build -Standard c89 -MainArguments
        if (!$?) {
            throw 'args build failed' 
        }
        & "$logs/args.exe" 'two words' 42
        if ($LASTEXITCODE) {
            throw 'args failed' 
        }
        & "$root/src/examples/minicurl/build.ps1" -BuildDirectory $build
        if (!$?) {
            throw 'minicurl build failed' 
        }
        & "$root/src/examples/minicurl/test.ps1"
        if (!$?) {
            throw 'minicurl local HTTP tests failed' 
        }
    }
    $compilerHash = (Get-FileHash -LiteralPath "$build/host/c1-stage2.exe").Hash
    if ($compilerHash -ne 'DB92844256433C7020D0C399E8F32D9CB9BE4954CC4063EAC0DFEAD0A4570ADD') {
        throw 'Final compiler differs from release baseline' 
    }
    [pscustomobject]@{Status = 'PASS'
        Mode = 'Full'
        TestsRun = $true
        StartedUtc = $started.ToString('o')
        FinishedUtc = [DateTime]::UtcNow.ToString('o')
        CompilerSHA256 = $compilerHash
        PowerShell = $PSVersionTable.PSVersion.ToString()
        OS = [Environment]::OSVersion.VersionString
        Scope = 'Full seed build, full audit, three supplemental suites/audits, examples and local HTTP tests'
    } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logs 'RESULT.json') -Encoding utf8
    Write-Host 'PASS release build and verification. Final compiler: src/bootstrap-c99/build-from-seed/host/c1-stage2.exe'
}
finally {
    Pop-Location 
}
