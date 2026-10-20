[简体中文](USAGE.zh-CN.md) · English

# Using the C1 compiler and PE linker

Use the driver for single-source programs. For multiple source files or additional Windows DLL imports, invoke the compiler and linker directly.

## Prepare the tools

Run these examples from the release root in **Windows PowerShell 5.1**, after successfully running `build-all.ps1` in any mode. The source package contains no precompiled programs. Build instructions are in [README.md](README.md).

```powershell
$ErrorActionPreference = 'Stop'
$snapshot = (Resolve-Path './src/bootstrap-c99').Path
$build = Join-Path $snapshot 'build-from-seed'
$compiler = Join-Path $build 'host/c1-stage2.exe'
$linker = Join-Path $build 'link-stage2.exe'
foreach ($tool in $compiler, $linker) {
    if (!(Test-Path -LiteralPath $tool)) { throw "Complete the build first: $tool" }
}
New-Item -ItemType Directory -Force './work/usage' | Out-Null
```

Later blocks reuse these variables in the same session. Repeating them overwrites example sources and artifacts with the same names.

## Compile a program with the driver

`compile-program.ps1` configures headers, compiles an object, selects the runtime, and links:

```powershell
@'
#include <stdio.h>
int main(void) {
    puts("Hello from C1!");
    return 0;
}
'@ | Set-Content -LiteralPath './work/usage/hello.c' -Encoding ascii

& "$snapshot/compile-program.ps1" `
    -Source './work/usage/hello.c' `
    -Output './work/usage/hello.exe' `
    -BuildDirectory $build -Standard c89
if (!$?) { throw 'Compilation or linking failed' }
& './work/usage/hello.exe'
if ($LASTEXITCODE -ne 0) { throw "Program exit code: $LASTEXITCODE" }
```

Expected output: `Hello from C1!`.

| Driver option | Meaning |
| --- | --- |
| `-Source` | Required: path to one C source file |
| `-Output` | Required: output path with an `.exe` extension |
| `-BuildDirectory` | Built toolchain directory; this page uses `build-from-seed`. The script defaults to `build` under its source directory, so specify it explicitly here |
| `-Standard c89` or `c99` | Language mode; driver defaults to `c89`. Choosing `c99` does not imply complete C99 support |
| `-MainArguments` | Builds a startup runtime for `int main(int argc, char **argv)` |
| `-Subsystem console` or `windows` | PE subsystem, default `console`; `windows` starts GUI programs without automatically allocating a console |
| `-Snapshot` | Source/header directory; normally omitted, defaults to the driver's directory |

For example, compile the included argument example:

```powershell
& "$snapshot/compile-program.ps1" `
    -Source "$snapshot/examples/args.c" `
    -Output './work/usage/args.exe' `
    -BuildDirectory $build -Standard c89 -MainArguments
if (!$?) { throw 'Compilation failed' }
& './work/usage/args.exe' 'two words' 42
```

The driver creates a `.c1-build-*` directory beside the output program, retaining intermediate sources and objects for inspection. Remove it when no longer needed. Extra DLL imports require direct linker invocation.

## Compiler executable interface

`host/c1-stage2.exe` **reads C source from standard input** and **writes binary data to standard output**; diagnostics go to standard error. It does not accept source filenames or GCC-style `-c`, `-o`, `-I`, or `-std` options. It waits until the input stream is closed.

| Environment variable | Accepted values and purpose |
| --- | --- |
| `C1_STANDARD` | `c89` or `c99`. Unset selects an internal default, not an explicit standards choice; always set it for applications |
| `C1_OUTPUT` | `object` emits custom `.c1o`; unset emits PE directly. Other nonempty values are errors; `exe` is invalid |
| `C1_INCLUDE` | Semicolon-separated header directories; absolute paths recommended |
| `C1_RUNTIME` | Unset for ordinary applications. `c89` selects runtime-build export policy and requires `C1_OUTPUT=object`; it is not a language-mode switch |

Direct PE output serves bootstrapping and self-contained programs; it does not automatically link the full standard library. Programs using that library must link `.c1o` objects with the runtime.

Windows PowerShell 5.1 text redirection and pipelines, including `>`, `Out-File`, and `Set-Content`, must not be used to capture binary output. The examples use `Set-Content` only for C text. Use the raw-byte-stream `Compile` function for compilation:

```powershell
. "$snapshot/tools/compile.ps1"
# Interface: Compile full-tool-path source-path output-path
# Failure throws an exception; do not use $LASTEXITCODE to check this function.
```

Input and output paths must differ. A failed call may leave an empty or partial file; require successful completion.

## Separate compilation and linking: a complete example

This compiles two C files separately and links the C89 runtime, restoring compilation environment variables afterward.

```powershell
@'
int add(int a, int b) { return a + b; }
'@ | Set-Content './work/usage/add.c' -Encoding ascii
@'
#include <stdio.h>
int add(int a, int b);
int main(void) {
    printf("sum = %d\n", add(20, 22));
    return 0;
}
'@ | Set-Content './work/usage/main.c' -Encoding ascii

. "$snapshot/tools/compile.ps1"
$out = (Resolve-Path './work/usage').Path
$saved = @{}
foreach ($name in 'C1_INCLUDE','C1_STANDARD','C1_OUTPUT','C1_RUNTIME') {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:C1_INCLUDE = ((@(
        'include','components/wide-input','components/wide-io',
        'components/wctype','components/inttypes','base/include'
    ) | ForEach-Object { Join-Path $snapshot $_ }) -join ';')
    $env:C1_STANDARD = 'c89'
    $env:C1_OUTPUT = 'object'
    $env:C1_RUNTIME = $null

    Compile $compiler "$out/add.c" "$out/add.c1o"
    Compile $compiler "$out/main.c" "$out/main.c1o"

    $runtime = Join-Path $build 'namespace/runtime-policy2/runtime.c1o'
    if (!(Test-Path -LiteralPath $runtime)) {
        throw 'C89 runtime missing; complete build-all.ps1'
    }
    $linkArgs = @(
        "$out/sum.exe", '__c1_start',
        "$out/main.c1o", "$out/add.c1o", $runtime
    )
    foreach ($api in (Get-Content "$snapshot/runtime/imports.json" -Raw | ConvertFrom-Json)) {
        $linkArgs += @('--import', ('__c1_win_' + $api), 'KERNEL32.dll', $api)
    }
    & $linker @linkArgs
    if ($LASTEXITCODE -ne 0) { throw "Link failed: $LASTEXITCODE" }
} finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name])
    }
}
& "$out/sum.exe"
if ($LASTEXITCODE -ne 0) { throw "Program exit code: $LASTEXITCODE" }
```

Expected output: `sum = 42`. Functions in other files are referenced through declarations, and the linker resolves their symbols.

For C99, set the application mode to `c99` and use `$build/host/runtime2.c1o`. These prebuilt runtimes serve examples without `argc/argv`. For command-line arguments, use `-MainArguments` or rebuild the appropriate startup runtime as shown in minicurl's build script.

The walkthrough's Toolchain stage produces `host/runtime2.c1o`, but not `namespace/runtime-policy2/runtime.c1o`, which this page's C89 examples need. minicurl builds its own runtime and can use those stage tools directly; this page assumes `build-all.ps1` outputs.

## Linker executable interface

Invocation:

```text
link-stage2.exe output.exe entry-symbol input.c1o ... [--subsystem console|windows] [--import symbol DLL export|#ordinal]
```

| Argument | Meaning |
| --- | --- |
| `output.exe` | Output PE path; the linker writes the file itself, not through standard output |
| `entry-symbol` | A globally defined entry symbol in an input object; normally `__c1_start` with this runtime |
| `input.c1o ...` | One or more project object files, including the program and required runtime |
| `--subsystem console` or `windows` | Console or GUI subsystem; default `console`. Specify once only; names are case-sensitive |
| `--import symbol DLL export` | Binds external `symbol` to the named export in the DLL |
| `--import symbol DLL #ordinal` | Import by ordinal, in the range 1–65535 |

`--import` may be repeated. Quote ordinal arguments in PowerShell, for example `'#123'`, so they are not treated as comments. Quote paths with spaces or use an argument array as in the complete example.

The runtime entry `__c1_start` handles startup and cannot simply be replaced by `main`. Applications must import the Windows APIs they call in addition to the runtime's `__c1_win_*` imports.

## Calling Windows DLLs

Declare the Windows API function in the application and explicitly import it. For an object referencing `GetCurrentProcessId`, append this before linking:

```powershell
$linkArgs += @('--import', 'GetCurrentProcessId', 'KERNEL32.dll', 'GetCurrentProcessId')
```

The corresponding declaration is `unsigned int GetCurrentProcessId(void);`. Imports only bind symbols; prototypes, arguments, return values, and structure layouts must match the target API.

[minicurl](src/examples/minicurl/README.md)'s `build.ps1` provides a complete example with an argument-enabled runtime and WINHTTP.dll imports.

The tools use custom `.c1o` files, not COFF `.obj` or SDK `.lib` files, and supply neither general `windows.h` compatibility nor a GCC/MSVC command-line layer. The linker produces EXEs only, not DLLs.

## Diagnostics and common problems

| Symptom | What to check |
| --- | --- |
| Compiler or runtime not found | Ensure `build-all.ps1` succeeded in any mode and `-BuildDirectory` points to the correct directory |
| Compiler keeps waiting | Supply standard input and close it; use `Compile` |
| EXE cannot run or object is invalid | Check for text redirection of binary output and for compilation failure |
| Header not found | Check `C1_INCLUDE`; top-level source arrives through stdin, so use the correct working directory or explicitly add project header directories |
| `C1_OUTPUT must be object or unset` | Set `object` or clear it; do not use `exe` |
| `C1_RUNTIME=c89 requires object output` | Clear it for applications; runtime policy requires object mode |
| `cannot read valid object` | Ensure input is a complete `.c1o` generated by these tools |
| `entry symbol has no global definition` | Check spelling and include the object defining the entry |
| `invalid or duplicate symbol definition` | Check duplicate definitions, repeated runtime objects, and import-symbol collisions |

The compiler and linker return 0 on success and nonzero on failure. Check `$LASTEXITCODE` after directly running an EXE; the driver and `Compile` report failure as PowerShell errors. Do not run an old same-named EXE after linking fails.

Language, library, and resource boundaries are in [C89-IMPLEMENTATION.md](src/C89-IMPLEMENTATION.md) and [C89-LIMITS.md](src/C89-LIMITS.md).

## GUI subsystem

Pass `-Subsystem windows` to the driver or `--subsystem windows` to the linker. The linker writes PE Subsystem 2 for Windows GUI or 3 for console; see the [Microsoft PE specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#windows-subsystem). The entry can remain `__c1_start` calling `main`; selecting the subsystem does not require WinMain.

Subsystem selection does not create a window or change standard-library behavior. GUI applications create their own windows and cannot rely on an automatically allocated console for stdout/stderr. The [Mario example](src/examples/mario/README.md) demonstrates a fixed-step game loop and GDI double buffering.
