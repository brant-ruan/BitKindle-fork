param([Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../../tools/compile.ps1')
$cases = [ordered]@{
    'missing' = 'inline int f(void){return 42;}int main(void){return f()!=42;}'
    'duplicate-a' = 'extern inline int f(void){return 42;}int main(void){return f()!=42;}'
    'duplicate-b' = 'extern inline int f(void){return 42;}'
    'private-a' = 'static inline int f(void){static int n;return ++n;}int a(void){return f();}int (*pa(void))(void){return f;}'
    'private-b' = 'static inline int f(void){static int n;return ++n;}int b(void){return f();}int (*pb(void))(void){return f;}'
    'private-main' = 'int a(void);int b(void);int (*pa(void))(void);int (*pb(void))(void);int main(void){if(pa()==pb())return 1;if(a()!=1||b()!=1||a()!=2||b()!=2)return 2;return 0;}'
}
$saved = $env:C1_OUTPUT
try {
    foreach ($g in 1, 2) {
        $root = Join-Path $BuildDirectory "features/inline/edges$g"
        New-Item -ItemType Directory -Force $root | Out-Null
        $compiler = Join-Path $BuildDirectory "host/c1-stage$g.exe"
        $env:C1_OUTPUT = 'object'
        foreach ($name in $cases.Keys) {
            $src = Join-Path $root "$name.c"
            [IO.File]::WriteAllText($src, $cases[$name])
            Compile $compiler $src (Join-Path $root "$name.c1o")
        }
        $env:C1_OUTPUT = $null
        foreach ($kind in 'missing', 'duplicate', 'private') {
            $objects = switch ($kind) {
                'missing' {
                    @('missing')
                }
                'duplicate' {
                    @('duplicate-a', 'duplicate-b')
                }
                'private' {
                    @('private-a', 'private-b', 'private-main')
                }
            }
            $exe = Join-Path $root "$kind.exe"
            $err = Join-Path $root "$kind.stderr"
            $line = '"' + $exe + '" main ' + (($objects | ForEach-Object { '"' + $root + '/' + $_ + '.c1o"' }) -join ' ')
            $p = Start-Process -FilePath (Join-Path $BuildDirectory "link-stage$g.exe") -ArgumentList $line -WindowStyle Hidden -PassThru -RedirectStandardError $err
            [void]$p.Handle
            if (!$p.WaitForExit(30000)) {
                $p.Kill()
                throw 'Link timeout'
            }
            $p.WaitForExit()
            if ($kind -ne 'private') {
                $expected = if ($kind -eq 'missing') {
                    'unresolved'
                }
                else {
                    'duplicate'
                }
                if ($p.ExitCode -ne 1 -or [IO.File]::ReadAllText($err) -notmatch $expected) {
                    throw "Wrong diagnostic: $kind"
                }
            }
            else {
                if ($p.ExitCode) {
                    throw 'Private inline link failed'
                }
                $p = Start-Process -FilePath $exe -WindowStyle Hidden -PassThru
                [void]$p.Handle
                if (!$p.WaitForExit(30000)) {
                    $p.Kill()
                    throw 'Execution timeout'
                }
                $p.WaitForExit()
                if ($p.ExitCode) {
                    throw 'Static inline identity/state failure'
                }
            }
            "PASS generation $g $kind inline link behavior"
        }
    }
    foreach ($name in (@($cases.Keys | ForEach-Object { $_ + '.c1o' }) + @('private.exe'))) {
        $a = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "features/inline/edges1/$name"))
        $b = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory "features/inline/edges2/$name"))
        if (!$a.Length -or $a.Length -ne $b.Length) {
            throw 'Length mismatch'
        }
        for ($i = 0; $i -lt $a.Length; $i++) {
            if ($a[$i] -ne $b[$i]) {
                throw "Bytes differ $name"
            }
        }
        "PASS identical $name"
    }
}
finally {
    $env:C1_OUTPUT = $saved
}
