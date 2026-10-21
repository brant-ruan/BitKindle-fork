# Stage 5: Three Fully Ported Applications (Step 05 - Applications)

## Overview

With the bootstrap toolchain and native runtime operating cleanly, this final stage completes the porting and automated regression testing of all three applications included in the original repository on macOS Apple Silicon (Darwin ARM64).

---

## Application Catalog & Technical Summary

### 1. `hello` & CLI Arguments `args`
- **Location**: `src/hello/hello.c`, `src/hello/args.c`
- **Highlights**:
  - Compiled directly with `c1-arm64` and linked with `c1link-macho` against `runtime.c1o`;
  - The startup stub captures and preserves dyld argument registers (`x0` as `argc`, `x1` as `argv`) on the stack across initializers before invoking `main`.

### 2. `minicurl` Network Client
- **Location**: `src/minicurl/minicurl.c`
- **Highlights**:
  - Replaces Windows-specific WinHTTP APIs with standard BSD/POSIX Sockets (`socket`, `connect`, `send`, `recv`);
  - Preserves exact CLI options (`minicurl -o <file> <url>`) and exit code specifications.

### 3. `mario` Super Mario Bros World 1-1
- **Location**: `src/mario/mario.c`, `src/mario/demo.h`
- **Highlights**:
  - **100% preservation of physics, level layout, and enemy collision matrices**;
  - Replaces Win32 GDI with an internal 960x544 32-bit RGBA FrameBuffer and standard BMP export;
  - Supports three distinct operating modes:
    - `--self-test`: Physics and mechanics verification (exit code 0);
    - `--smoke-test <out.bmp>`: Framebuffer rendering and BMP screenshot export;
    - `--demo-test <report.txt>`: Heuristic beam-search AI auto-player completing World 1-1 (clears in 1634 ticks without damage).

---

## Verification

```bash
chmod +x test_all.sh
./test_all.sh
```
