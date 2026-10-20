param([int]$Generation = 1, [Parameter(Mandatory = $true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../tools/compile.ps1')
$root = Join-Path $BuildDirectory ('integration/runtime' + $Generation)
$compiler = Join-Path $BuildDirectory ('host/c1-stage' + $Generation + '.exe')
$linker = Join-Path $BuildDirectory ('link-stage' + $Generation + '.exe')
$apis = Get-Content ([IO.Path]::GetFullPath('src/bootstrap-c99/runtime/imports.json')) -Raw | ConvertFrom-Json
$options = ($apis | ForEach-Object { '--import __c1_win_' + $_ + ' KERNEL32.dll ' + $_ }) -join ' '
function Run([string]$Exe, [string]$Arguments, [int]$Expected, [string]$Label) {
    $p = Start-Process -FilePath $Exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root ($Label + '.stdout')) -RedirectStandardError (Join-Path $root ($Label + '.stderr'))
    [void]$p.Handle
    if (-not $p.WaitForExit(30000)) {
        $p.Kill()
        throw "Timeout $Label"
    }
    $p.WaitForExit()
    if ($p.ExitCode -ne $Expected) {
        throw "$Label returned $($p.ExitCode), expected $Expected; $([IO.File]::ReadAllText((Join-Path $root ($Label+'.stderr'))))"
    }
}
$cases = @(
    @('hello', 'void', ("#include <stdio.h>`n" + 'int main(void){printf("hello\n");return 42;}'), 42, ' ', "hello`r`n"),
    @('cleanup', 'void', ("#include <stdio.h>`n#include <stdlib.h>`n" + 'void first(void){putchar(65);}void second(void){putchar(66);}int main(void){atexit(first);atexit(second);printf("body");return 42;}'), 42, ' ', 'bodyBA'),
    @('explicit-exit', 'void', ("#include <stdio.h>`n#include <stdlib.h>`n" + 'void handler(void){putchar(90);}int main(void){atexit(handler);exit(23);return 1;}'), 23, ' ', 'Z'),
    @('args', 'args', ("#include <stdio.h>`n#include <string.h>`n" + 'int main(int argc,char **argv){if(argc!=3||strcmp(argv[1],"two words")||strcmp(argv[2],"x")||argv[3])return 1;printf("%s:%s",argv[1],argv[2]);return 42;}'), 42, '"two words" x', 'two words:x'),
    @('memory-math', 'void', ("#include <stdlib.h>`n#include <string.h>`n#include <math.h>`n" + 'int main(void){char *p=malloc(32);if(!p)return 1;strcpy(p,"42");if(strtol(p,0,10)!=42||pow(9,1.5)!=27||fabs(sin(1))<0.8)return 2;p=realloc(p,64);if(!p||strcmp(p,"42"))return 3;free(p);return 42;}'), 42, ' ', ''),
    @('temporary-file', 'void', ("#include <stdio.h>`n#include <string.h>`n" + 'int main(void){FILE *f=tmpfile();char b[8];if(!f)return 1;if(fputs("hello",f)<0)return 2;rewind(f);if(!fgets(b,8,f)||strcmp(b,"hello"))return 3;if(fclose(f))return 4;return 42;}'), 42, ' ', '')
)
$savedInclude = $env:C1_INCLUDE
$savedOutput = $env:C1_OUTPUT
try {
    $env:C1_INCLUDE = (@('include', 'components/wide-input', 'components/wide-io', 'components/wctype', 'components/inttypes', 'base/include') | ForEach-Object { [IO.Path]::GetFullPath('src/bootstrap-c99/' + $_) }) -join ';'
    foreach ($case in $cases) {
        $name = $case[0]
        $source = Join-Path $root ($name + '.c')
        $obj = Join-Path $root ($name + '.c1o')
        $exe = Join-Path $root ($name + '.exe')
        [IO.File]::WriteAllText($source, $case[2], [Text.Encoding]::ASCII)
        $env:C1_OUTPUT = 'object'
        Compile $compiler $source $obj
        $env:C1_OUTPUT = $null
        $runtime = Join-Path $root ('runtime-' + $case[1] + '.c1o')
        Run $linker ('"' + $exe + '" __c1_start "' + $obj + '" "' + $runtime + '" ' + $options) 0 ($name + '-link')
        Run $exe $case[4] $case[3] $name
        if ([IO.File]::ReadAllText((Join-Path $root ($name + '.stdout'))) -cne $case[5]) {
            throw "Unexpected stdout $name"
        }
        if ([IO.File]::ReadAllText((Join-Path $root ($name + '.stderr'))) -cne '') {
            throw "Unexpected stderr $name"
        }
        "PASS hosted object runtime $name"
    }
}
finally {
    $env:C1_INCLUDE = $savedInclude
    $env:C1_OUTPUT = $savedOutput
}
