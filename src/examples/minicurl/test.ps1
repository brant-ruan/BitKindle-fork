param([switch]$Internet)
. (Join-Path $PSScriptRoot '../../bootstrap-c99/base/ps51-compat.ps1')

$ErrorActionPreference = 'Stop'
$exe = Join-Path $PSScriptRoot 'minicurl.exe'
$scratch = Join-Path $PSScriptRoot 'build/tests'
New-Item -ItemType Directory -Force $scratch | Out-Null
$ready = Join-Path $scratch ('ready-' + [Guid]::NewGuid().ToString('N') + '.txt')
$job = Start-Job -ArgumentList $ready -ScriptBlock {
    param($ready)
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $listener.Start()
    [IO.File]::WriteAllText($ready, [string]$listener.LocalEndpoint.Port)
    try {
        $deadline = [DateTime]::UtcNow.AddMinutes(3)
        while ([DateTime]::UtcNow -lt $deadline -and !(Test-Path -LiteralPath ($ready + '.stop'))) {
            if (!$listener.Pending()) {
                Start-Sleep -Milliseconds 50
                continue 
            }
            $client = $listener.AcceptTcpClient()
            try {
                $stream = $client.GetStream()
                $stream.ReadTimeout = 10000
                $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::ASCII, $false, 1024, $true)
                $first = $reader.ReadLine()
                do {
                    $line = $reader.ReadLine() 
                } while ($line)
                $route = ($first -split ' ')[1]
                $status = '200 OK'
                $extra = ''
                $body = [Text.Encoding]::UTF8.GetBytes("hello from self-built C compiler`n")
                if ($route -eq '/redirect') {
                    $status = '302 Found'
                    $extra = "Location: /hello`r`n"
                    $body = [byte[]]@() 
                }
                elseif ($route -eq '/missing') {
                    $status = '404 Not Found'
                    $body = [Text.Encoding]::ASCII.GetBytes('missing') 
                }
                elseif ($route -eq '/binary') {
                    $body = [byte[]](0..255) * 200 
                }
                elseif ($route -eq '/chunked') {
                    $wire = [Text.Encoding]::ASCII.GetBytes("HTTP/1.1 200 OK`r`nTransfer-Encoding: chunked`r`nConnection: close`r`n`r`n5`r`nhello`r`n6`r`n world`r`n0`r`n`r`n")
                    $stream.Write($wire, 0, $wire.Length)
                    continue
                }
                elseif ($route -ne '/hello') {
                    $body = [Text.Encoding]::ASCII.GetBytes($route) 
                }
                $headers = [Text.Encoding]::ASCII.GetBytes("HTTP/1.1 $status`r`n${extra}Content-Length: $($body.Length)`r`nConnection: close`r`n`r`n")
                $stream.Write($headers, 0, $headers.Length)
                $stream.Write($body, 0, $body.Length)
            }
            finally {
                $client.Dispose() 
            }
        }
    }
    finally {
        $listener.Stop() 
    }
}
$results = [Collections.Generic.List[string]]::new()
function Run-Case([string]$name, [string[]]$arguments, [int]$expected) {
    $info = [Diagnostics.ProcessStartInfo]::new($exe)
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.Arguments = ConvertTo-C1WindowsArguments $arguments
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $info
    $file = [IO.File]::Create((Join-Path $scratch ($name + '.stdout')))
    try {
        [void]$process.Start()
        $copy = $process.StandardOutput.BaseStream.CopyToAsync($file)
        $errors = $process.StandardError.ReadToEndAsync()
        if (!$process.WaitForExit(60000)) {
            $process.Kill()
            throw "Timeout: $name" 
        }
        [void]$copy.GetAwaiter().GetResult()
        [IO.File]::WriteAllText((Join-Path $scratch ($name + '.stderr')), $errors.GetAwaiter().GetResult())
        if ($process.ExitCode -ne $expected) {
            throw "$name returned $($process.ExitCode), expected $expected; see $scratch/$name.stderr" 
        }
    }
    finally {
        $file.Dispose()
        $process.Dispose() 
    }
    $results.Add("PASS $name (exit $expected)")
}
function Check-Bytes([string]$path, [byte[]]$expected) {
    $actual = [IO.File]::ReadAllBytes($path)
    if ($actual.Length -ne $expected.Length) {
        throw "Length mismatch: $path" 
    }
    for ($i = 0; $i -lt $actual.Length; $i++) {
        if ($actual[$i] -ne $expected[$i]) {
            throw "Byte mismatch at ${i}: $path" 
        } 
    }
}
try {
    for ($i = 0; $i -lt 100 -and !(Test-Path -LiteralPath $ready); $i++) {
        Start-Sleep -Milliseconds 100 
    }
    if (!(Test-Path -LiteralPath $ready)) {
        Receive-Job $job
        throw 'Test server failed to start' 
    }
    $port = [IO.File]::ReadAllText($ready)
    $url = "http://127.0.0.1:$port"
    $hello = [Text.Encoding]::UTF8.GetBytes("hello from self-built C compiler`n")
    Run-Case help @('--help') 0
    Run-Case usage @() 2
    Run-Case bad-scheme @('ftp://example.com/') 2
    Run-Case credentials @('http://user:pass@127.0.0.1/') 2
    Run-Case http @("$url/hello") 0
    Check-Bytes (Join-Path $scratch 'http.stdout') $hello
    Run-Case redirect @("$url/redirect") 0
    Check-Bytes (Join-Path $scratch 'redirect.stdout') $hello
    Run-Case chunked @("$url/chunked") 0
    Check-Bytes (Join-Path $scratch 'chunked.stdout') ([Text.Encoding]::ASCII.GetBytes('hello world'))
    Run-Case query @("${url}?q=a%20b#ignored") 0
    Check-Bytes (Join-Path $scratch 'query.stdout') ([Text.Encoding]::ASCII.GetBytes('/?q=a%20b'))
    Run-Case missing @("$url/missing") 22
    Check-Bytes (Join-Path $scratch 'missing.stdout') ([Text.Encoding]::ASCII.GetBytes('missing'))
    $binary = Join-Path $scratch 'download with spaces.bin'
    Run-Case binary-file @('-o', $binary, "$url/binary") 0
    Check-Bytes $binary ([byte[]](0..255) * 200)
    Run-Case binary-stdout @("$url/binary") 0
    Check-Bytes (Join-Path $scratch 'binary-stdout.stdout') ([byte[]](0..255) * 200)
    Run-Case output-error @('-o', $scratch, "$url/hello") 4
    if ($Internet) {
        $httpsArgs = @('-o', (Join-Path $scratch 'example.html'))
        if ($env:HTTPS_PROXY) {
            $proxyUri = [Uri]$env:HTTPS_PROXY
            if ($proxyUri.Scheme -ne 'http' -or $proxyUri.UserInfo) {
                throw 'Test supports only HTTP proxies without embedded credentials' 
            }
            $httpsArgs += @('--proxy', $proxyUri.Authority)
        }
        $httpsArgs += 'https://example.com/'
        Run-Case https $httpsArgs 0
        if ([IO.File]::ReadAllText((Join-Path $scratch 'example.html')) -notmatch 'Example Domain') {
            throw 'Unexpected HTTPS response' 
        }
    }
    $results.Add('PASS byte-for-byte bodies: HTTP, redirect, chunked, query/fragment, 404, binary file and stdout')
    $results | Set-Content (Join-Path $scratch 'RESULTS.txt') -Encoding utf8
    $results
}
finally {
    [IO.File]::WriteAllText(($ready + '.stop'), 'stop')
    $job | Wait-Job -Timeout 15 | Out-Null
    Stop-Job $job
    Remove-Job $job
    if (Test-Path -LiteralPath $ready) {
        Remove-Item -LiteralPath $ready 
    }
    if (Test-Path -LiteralPath ($ready + '.stop')) {
        Remove-Item -LiteralPath ($ready + '.stop') 
    }
}
