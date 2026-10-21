# BitKindle macOS (Darwin ARM64) Port & Implementation

> This project is a macOS Apple Silicon port based on the original [BitKindle](https://github.com/tombkeeper/BitKindle) created by **[tombkeeper](https://github.com/tombkeeper)**.
>
> **Model Attribution**:  
> The macOS Apple Silicon (Darwin ARM64) port, architecture adaptation, and test suites were implemented and verified with the assistance of the **Gemini 3.8 Flash (Medium)** model.

---

## 1. Project Background & Porting Motivation

BitKindle is a minimalist C self-hosting compilation system originally created for Windows x64. Its core approach starts from a small seed of raw hexadecimal machine code, incrementally constructing a linker, C compiler, and minimal runtime until the compiler can successfully build itself.

The original implementation targets Windows x64 (x86-64 ISA, PE32+ binary format, and Win32 APIs). This project ports that self-hosting workflow to **Apple Silicon (macOS Darwin ARM64)** to explore how the same bootstrapping approach operates under a different hardware and OS ecosystem:
- **CPU Architecture**: Transitioning from x86-64 CISC to ARM64 (AArch64) RISC, accommodating fixed 32-bit instructions and the AAPCS64 calling convention;
- **Executable Format**: Transitioning from PE32+ to 64-bit Mach-O, while complying with Apple Silicon's 16KB physical page alignment and AMFI code-signing requirements;
- **Syscalls & Runtime**: Removing Windows-specific APIs in favor of self-contained runtime routines built directly on raw Darwin `svc #0x80` system calls.

This port aims to provide a practical, transparent reference for developers and students exploring bootstrap compilation and low-level systems programming on modern Apple Silicon hardware.

---

## 2. Dual-Track Architecture

To serve both systems engineers seeking a streamlined master toolchain and learners studying step-by-step bootstrap ladders, the `macos-arm64/` directory provides a **Dual-Track** structure:

```text
macos-arm64/
├── build-all.sh                   # Master orchestrator running both tracks end-to-end
├── README.md                      # [Bilingual] English architectural overview
├── README.zh-CN.md                # [Bilingual] Chinese architectural overview
│
├── 01-top-down/                   # [High-Level Integrated View]
│   ├── build-all.sh               # 7-step automated build & regression pipeline
│   ├── src/                       # Modular source code (base, linker, compiler, runtime, examples)
│   ├── scripts/                   # Integration test suites
│   ├── README.md & .zh-CN.md      # Top-Down guide and quick start
│   └── .gitignore
│
└── 02-down-top/                   # [Progressive Ladder View (0 to N)]
    ├── build-all.sh               # Step-by-step master runner (Step 00 to Step 05)
    ├── step00-seed/               # Stage 0: Pure hex ARM64 machine code seed
    ├── step01-linker/             # Stage 1: Standalone 64-bit Mach-O linker & codesign (c1link-macho)
    ├── step02-compiler/           # Stage 2: Native ARM64 C1 recursive descent compiler (c1-arm64)
    ├── step03-runtime/            # Stage 3: Self-contained Darwin runtime & raw syscalls (runtime.c1o)
    ├── step04-self-hosting/       # Stage 4: Two-generation deterministic loop (Byte-for-byte SHA256)
    ├── step05-applications/       # Stage 5: Acceptance of all 3 ported applications
    ├── README.md & .zh-CN.md      # Down-Top notes and stage documentation
    └── .gitignore
```

1. **`01-top-down/` (Integrated View)**:
   Designed for engineering execution. Packages the seed, linker, compiler, runtime, and applications into an automated, cohesive pipeline that executes all build and test steps in seconds.
2. **`02-down-top/` (Ladder View)**:
   Faithfully reflects the incremental ethos of the original project. Each step is an isolated directory containing its own source code, self-contained test runner (`run.sh` / `build_and_test.sh`), and bilingual conceptual guides.

---

## 3. Deep Architectural Comparison: macOS (Darwin ARM64) vs Windows (PE32+ x64)

During the porting process, nine fundamental system-level differences between the two platforms were addressed:

| Dimension | Windows x64 (Original BitKindle) | macOS Darwin ARM64 (Ported Implementation) | Porting Solution & Architecture |
| :--- | :--- | :--- | :--- |
| **Binary Container** | **PE32+ (Portable Executable)**<br>DOS Stub + PE Sig + COFF Header + Optional Header + Sections (`.text`, `.data`) | **Mach-O 64-bit** (`MH_EXECUTE`)<br>`mach_header_64` + `LC_SEGMENT_64` (`__PAGEZERO`, `__TEXT`, `__DATA`, `__LINKEDIT`) | Developed `c1link-macho` from scratch to emit clean, valid 64-bit Mach-O binaries. |
| **Instruction Set Architecture (ISA)** | **x86-64 (AMD64)**<br>CISC, variable 1~15 byte instructions, memory-direct arithmetic operands. | **ARM64 (AArch64)**<br>RISC, uniform 32-bit (4-byte) instruction width, Load/Store architecture. | Implemented custom ARM64 instruction emitter, handling condition inversion and PC-relative address encodings. |
| **Calling Convention (ABI)** | **Microsoft x64 ABI**<br>First 4 parameters in `RCX, RDX, R8, R9`; **Mandatory 32-byte Shadow Space**; returns in `RAX`. | **AAPCS64**<br>First 8 integer parameters in `x0~x7`; returns in `x0`; `x29` is Frame Pointer (FP), `x30` is Link Register (LR). | Frame layout managed via `stp x29, x30, [sp, #-16]!` and `mov x29, sp`, preserving 16-byte stack alignment. |
| **Relocation Mechanisms** | **PE/COFF Relative Offsets**<br>`IMAGE_REL_AMD64_REL32` computing `Target - PC - 4`. | **ARM64 Page & Branch Relocations**<br>`LK_ARM64_PAGE21` (`adrp` page base); `LK_ARM64_PAGEOFF12` (12-bit intra-page offset); `LK_ARM64_BRANCH26` (26-bit call). | The linker features a two-pass resolution engine patching target symbols directly into instruction bitfields. |
| **Physical Page Alignment** | **Standard 4KB (0x1000)**<br>Virtual addresses and file sections aligned to 4096 or 512 bytes. | **Hardware-Enforced 16KB (0x4000)**<br>Apple Silicon M-series hardware strictly requires 16KB virtual memory alignment. | Segments (`__PAGEZERO`, `__TEXT`) are strictly 16KB-aligned; this is the physical reason the seed binary is padded to 34KB. |
| **Code Signing & AMFI** | **Authenticode (Optional)**<br>Standard user-mode executables execute freely without signatures. | **AMFI Kernel Enforcement**<br>Kernel security immediately issues `SIGKILL` on unsigned Mach-O binaries. | `c1link-macho` formats proper section tables and automatically signs outputs via `codesign -s - --force`. |
| **System Call Dispatch** | **Indirect Dispatch**<br>Delegated through `ntdll.dll`; direct syscall numbers are unstable across NT builds. | **Raw Darwin Traps (`svc #0x80`)**<br>BSD-style syscall numbers supplied in register `x16` (`x16=4` for `sys_write`, `x16=1` for `sys_exit`). | Assembled standalone `runtime.c1o` static runtime without external libc, interacting directly with the kernel via `svc`. |
| **Network Architecture** | **WinHTTP APIs**<br>High-level abstraction tied to `winhttp.dll`. | **Native BSD/POSIX Sockets**<br>Standard Berkeley Sockets (`socket`, `connect`, `send`, `recv`). | Ported `minicurl` using POSIX sockets for HTTP/1.0 GET requests and file streaming, preserving exact CLI switches. |
| **Graphics & Presentation** | **Win32 GDI Windows**<br>`CreateWindowExA`, `FillRect`, `GetPixel`, `GetDIBits`, and the Windows message loop. | **Decoupled 32-bit FrameBuffer + BMP Export**<br>Internal 960×544 RGBA buffer, 5×7 font engine, headless automation, and BMP image serialization. | Ported `mario`, retaining 100% of the original physics engine, level layout, and AI player, stripping GDI dependencies. |

---

## 4. Two-Generation Byte-for-Byte Determinism

In the self-hosting validation loop:
1. The Generation 0 compiler (`compiler_gen0`) compiles the core logic to emit the Stage 1 object file (`compiler_stage1.c1o`);
2. `c1link-macho` links Stage 1 into an executable (`app_stage1`), which compiles the same source code to emit Stage 2 (`compiler_stage2.c1o`).

On Darwin ARM64, both object files are precisely **336 bytes**, producing identical SHA256 checksums:

```text
Stage 1 SHA256: 7e9e070420fda3d32f40bd7ca3d6deb0cef9966ca55e783e297a6afec6f79bac
Stage 2 SHA256: 7e9e070420fda3d32f40bd7ca3d6deb0cef9966ca55e783e297a6afec6f79bac
Exit Code: 42 (PASS)
```

This confirms mathematical determinism and proves that the self-hosting loop is free of external compiler bias.

---

## 5. Ported Application Suite

All three applications from the original repository are ported, functional, and verified:

1. **`hello` & CLI Argument Parser `args`**:
   - Location: `01-top-down/src/examples/hello/` and `02-down-top/step05-applications/src/hello/`;
   - Mach-O startup stub preserves dyld input registers (`x0` as `argc`, `x1` as `argv`) onto the stack before calling `main`;
   - Output `./hello` returns `"hello"`; `./args "two words" "42"` parses correctly with exit code `0`.
2. **`minicurl` Network Client**:
   - Location: `src/examples/minicurl/minicurl.c`;
   - Uses POSIX sockets to download files (`minicurl -o <file> <url>`);
   - Verified against a local HTTP server with 100% byte-for-byte content matching.
3. **`mario` Super Mario Bros World 1-1**:
   - Location: `src/examples/mario/mario.c` and `demo.h`;
   - **All 3 verification modes pass**:
     - `--self-test`: Physics engine self-test (gravity, jumps, blocks, stomps, turtle shells pass with code 0);
     - `--smoke-test`: Exports `mario_smoke.bmp` (2.0MB, 960×544, viewable in macOS Preview);
     - `--demo-test`: Built-in heuristic beam-search AI completes World 1-1:
       ```text
       PASS state=2 ticks=1634 x=6339 coins=10 boxes=9 mushrooms=1 goombas=11 turtles=3 kicks=2 shellkills=0 loose=2 hurts=0
       ```
       (Clears the entire level in 1634 ticks without taking damage, reaching the flagpole to win).
4. **`mario-extended` Native macOS Cocoa Interactive Window Edition**:
   - Location: `src/examples/mario-extended/`;
   - **Zero Third-Party Dependencies**: Written purely in C and Objective-C linking system-native `-framework Cocoa`;
   - Spawns a native 960×544 game window running at 60 FPS with responsive keyboard controls (A/D to move, Space/W to jump, Shift to dash, P to pause, R to restart, ESC to quit);
   - Launch and play interactively on your Mac screen: `./01-top-down/src/examples/mario-extended/build.sh --run`;
   - Or watch the AI clear World 1-1 live in a window: `./01-top-down/src/examples/mario-extended/build.sh --demo`.

---

## 6. Quick Start

Run the automated test harnesses directly from within `macos-arm64/`:

```bash
# 1. Master Dual-Track Run (executes both 01-top-down and 02-down-top)
chmod +x build-all.sh
./build-all.sh

# 2. Run Integrated View only
cd 01-top-down
./build-all.sh

# 3. Run Step-by-Step Ladder View only
cd ../02-down-top
./build-all.sh
```
