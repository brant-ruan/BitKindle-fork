# Stage 3: Self-Contained Darwin Runtime & Standard I/O (Step 03 - Runtime)

## Overview

Programs written in C require interaction with the host operating system to emit characters and format numbers. Traditional toolchains delegate this responsibility to large dynamic runtime libraries supplied by the OS (e.g., `libSystem.dylib` on macOS or `glibc` on Linux).

BitKindle maintains **zero external runtime dependencies**. In this stage, we construct a native, self-contained Darwin ARM64 runtime module, `runtime.c1o`.

---

## File Structure

- `src/build_runtime.c`: Machine code generator that emits raw ARM64 system call wrappers and formatted I/O routines directly into a `.c1o` object file;
- `tests/hello.c`: Integration program verifying string emission, line feeds, and decimal integer formatting;
- `build_and_test.sh`: One-click script for runtime generation, cross-module Mach-O linking, and execution.

---

## Implementation Details

1. **Exported Runtime Symbols**:
   - `sys_write(int fd, const char *buf, int len)`: Dispatches via `svc #0x80` (`x16=4`);
   - `sys_exit(int code)`: Dispatches via `svc #0x80` (`x16=1`);
   - `putchar(int c)`: Emits a single character to standard output;
   - `print(const char *s)`: Iteratively prints a null-terminated string;
   - `puts(const char *s)`: Emits a string appended with a newline;
   - `print_int(int val)`: Formats and prints 32-bit signed integers in decimal notation.
2. **Cross-Module Symbol Resolution**:
   - `c1link-macho` merges `hello.c1o` (produced by `c1-arm64`) and `runtime.c1o`, resolving external symbol references and performing final address fixups.

---

## Verification

```bash
chmod +x build_and_test.sh
./build_and_test.sh
```
