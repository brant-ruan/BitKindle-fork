# BitKindle macOS (Darwin ARM64) - Integrated Top-Down Architecture (01-top-down)

This directory provides the Apple Silicon (macOS Darwin ARM64) port of BitKindle, based on the original Windows x64 self-hosting project created by tombkeeper.

`01-top-down` represents the **engineering-grade, cohesive, and end-to-end integrated** system view: the bootstrap compiler, linker, runtime, and all three application suites work seamlessly together in an automated production pipeline.

---

## Directory Structure

```text
01-top-down/
├── build-all.sh               # One-click master build & verification pipeline (Steps 1 ~ 7)
├── scripts/                   # Individual unit tests and regression test harnesses
│   ├── check_env.sh           # Host environment check (Darwin, arm64, clang, codesign)
│   ├── decode_seed.sh         # Pure hexadecimal machine code seed decoder
│   ├── test_linker.sh         # Mach-O linker relocations & ad-hoc code signing tests
│   ├── test_compiler.sh       # c1-arm64 recursive descent code generation tests
│   ├── test_runtime.sh        # Standalone Darwin runtime & raw syscall I/O verification
│   ├── test_self_host.sh      # Two-generation byte-for-byte deterministic compiler loop (SHA256)
│   ├── test_minicurl.sh       # POSIX Socket HTTP client fetch & file writing test
│   └── test_all_examples.sh   # Integrated regression suite for all 3 ported applications
└── src/                       # Core source code
    ├── base/                  # Minimal ARM64 machine code seed (seed-arm64.hex)
    ├── linker/                # Standalone 64-bit Mach-O linker (link.c, macho.c, etc.)
    ├── compiler/              # Native ARM64 C compiler backend (c1_arm64.c)
    ├── runtime/               # Darwin standalone runtime generator (build_runtime.c)
    └── examples/              # Three ported applications
        ├── hello/             # hello.c & command-line argument parser args.c
        ├── minicurl/          # Native POSIX HTTP/1.0 client (minicurl.c)
        └── mario/             # Super Mario Bros World 1-1 game engine (mario.c, demo.h)
```

---

## Quick Start

Run the automated master pipeline from within this directory:

```bash
./build-all.sh
```

The pipeline automatically carries out the following 7 steps:
1. **[Step 1/7] Environment Check**: Validates Darwin arm64 OS kernel, Apple Clang, and codesign utilities;
2. **[Step 2/7] Pure Hex Seed Decoding**: Decodes `seed-arm64.hex` to a native Mach-O executable seed and tests standard I/O pipes;
3. **[Step 3/7] Toolchain Compilation**: Builds `build/c1link-macho` and `build/c1-arm64`;
4. **[Step 4/7] Darwin Runtime Generation**: Emits raw Darwin syscall wrappers (`sys_write`, `sys_exit`, `print_int`), outputting `build/runtime.c1o`;
5. **[Step 5/7] Unit Regression**: Verifies Mach-O relocations, compiler arithmetic/loop code generation, and standard console I/O programs;
6. **[Step 6/7] Two-Generation Deterministic Self-Hosting Loop**: Gen0 emits Stage1 object file, Stage1 self-compiles to emit Stage2, validating identical SHA256 checksums byte-for-byte;
7. **[Step 7/7] Ported Applications Acceptance**:
   - `hello` & `args`: Compiled & linked using custom `c1-arm64` and `c1link-macho`, verifying native dyld argc/argv forwarding;
   - `minicurl`: Launches local HTTP test server and verifies payload retrieval over POSIX sockets;
   - `mario`: Runs headless physics verification (`--self-test`), exports 2.0MB BMP screenshot (`--smoke-test`), and verifies AI heuristic auto-play completion of World 1-1 (`--demo-test`).

---

## Architectural Highlights

1. **Mach-O ARM64 Linking & Code Signing**:
   - Fully conforms to Apple Silicon 16KB physical page alignment (`0x4000`);
   - Emits correct AAPCS64 relocation types (`PAGE21`, `PAGEOFF12`, `BRANCH26`);
   - Automatically signs executables with `codesign -s - --force` to satisfy Darwin AMFI kernel security requirements.
2. **AAPCS64 Frame & Register Conventions**:
   - Maintains strict 16-byte stack pointer alignment;
   - Manages frame records via `stp x29, x30, [sp, #-16]!` and `mov x29, sp`;
   - Passes the first 8 integer arguments via `x0` ~ `x7` and receives return values in `x0`.
3. **Raw Darwin Syscalls**:
   - Dispatches syscalls via `svc #0x80` with BSD-style syscall numbers in register `x16` (`x16=4` for `sys_write`, `x16=1` for `sys_exit`).
