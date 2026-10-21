# BitKindle macOS (Darwin ARM64) Porting Roadmap

This roadmap documents the comprehensive port of the BitKindle minimalist C self-hosting compilation and bootstrap toolchain from Windows x64 to Apple Silicon (macOS Darwin ARM64). Executed progressively via "Top-down verification with Bottom-up closure" (Strategy B), all phases have achieved 100% completion and successful delivery.

---

## Phase Evolution Overview

```text
Phase 1: Environment & Prototype ──► Phase 2: Mach-O Linker ──► Phase 3: ARM64 Compiler Backend
                                                                           │
Phase 6: Application Suite Port ◄── Phase 5: Deterministic Closure ◄── Phase 4: Darwin Standalone Runtime
          │
          ▼
Phase 7: Dual-Track Architecture (Top-Down / Down-Top) & Bilingual Docs
          │
          ▼
Phase 8: Native macOS Cocoa Interactive Window (mario-extended)
```

---

## Detailed Milestone Records

### Phase 1: Infrastructure & Minimal Mach-O Prototype Verification
- [x] Established the `macos-arm64/` standalone project scaffold and environment detector (`check_env.sh`).
- [x] Developed native macOS build and verification scripts (POSIX / Zsh).
- [x] Implemented and validated a minimal standalone Mach-O + ARM64 binary generator prototype (verifying direct byte-stream assembly and code-signing on Apple Silicon).

### Phase 2: Mach-O Object File Linker (`c1link-macho`)
- [x] Ported and adapted `.c1o` object file readers and writers (supporting ARM64 `0xAA64`).
- [x] Implemented `macho.c`, emitting native 64-bit Mach-O segment and section tables (`__PAGEZERO`, `__TEXT`, `__DATA`, `__LINKEDIT`).
- [x] Enforced Apple Silicon hardware-mandated 16KB physical page alignment (`0x4000`).
- [x] Implemented AAPCS64 relocation types (`LK_ARM64_BRANCH26`, `LK_ARM64_PAGE21`, `LK_ARM64_PAGEOFF12`).
- [x] Integrated automated Apple AMFI kernel code signing (`codesign -s - --force`) and synthesized standard ARM64 startup stubs.
- [x] Passed automated end-to-end integration tests (`test_linker.sh`), validating object emission, linking, and execution (Exit Code: 42).

### Phase 3: Compiler Backend ARM64 Code Generator (`c1-arm64`)
- [x] Developed a dedicated C compiler frontend and ARM64 code emitter (`c1_arm64.c`).
- [x] Implemented AAPCS64 frame layout (`stp/ldp x29, x30`, strictly maintaining 16-byte stack alignment).
- [x] Implemented ARM64 arithmetic (`add`, `sub`, `mul`, `sdiv`), condition setting (`cset`), and branch control flow (`cbz`, `b`, `bl`).
- [x] Supported multi-argument function calls and register argument passing adhering to the Apple Silicon ABI (`x0`~`x7`).
- [x] Completed the entire `C source -> c1-arm64 -> .c1o object -> c1link-macho -> Mach-O executable -> run` pipeline (arithmetic and loop calculations returning 42).

### Phase 4: Darwin Runtime & Standard I/O (`runtime-darwin`)
- [x] Developed a self-contained Darwin runtime generator (`build_runtime.c`), emitting `runtime.c1o`.
- [x] Implemented system I/O and process exit using raw Darwin syscalls (`svc #0x80`, syscall number in `x16`: `sys_write`, `sys_exit`).
- [x] Implemented and exported fundamental standard I/O routines (`putchar`, `print`, `puts`, `print_int`).
- [x] Enhanced `c1-arm64` with string literal pool management and external unresolved symbol exports (`LK_UNDEFINED`).
- [x] Completed automated cross-module static linking and console standard I/O verification (formatting output with return code 42).

### Phase 5: Self-Hosting Closure & Minimal Machine Code Seed
- [x] Handcrafted and generated the Darwin ARM64 minimal machine code seed (`seed-arm64.hex`).
- [x] Implemented a POSIX seed decoder and signature driver (`decode_seed.sh`), validating seed execution from pure hexadecimal text.
- [x] Achieved two-generation self-hosting (`bootstrap_core.c`), emitting Generation 1 and Generation 2 object files.
- [x] Executed byte-for-byte determinism checks (`test_self_host.sh`): both object files are precisely 336 bytes with identical SHA256 hashes (`7e9e070420...`), exiting with code 42.

### Phase 6: Application Suite Porting
- [x] **`hello` & CLI Argument Parser `args`**: Enhanced the linker startup stub to preserve and forward dyld argument registers (`x0` as `argc`, `x1` as `argv`), added pointer declaration support in `c1-arm64`, validated with exit code 0.
- [x] **`minicurl` Network Client**: Replaced Windows WinHTTP APIs with native BSD/POSIX Sockets (`socket`, `connect`, `send`, `recv`), implementing HTTP/1.0 GET streaming with 100% byte-for-byte local payload match.
- [x] **`mario` Super Mario Bros World 1-1**: 100% preservation of original physics, level layout, and enemy collision AI; decoupled Win32 GDI with an internal 32-bit FrameBuffer and BMP serializer; verified across `--self-test`, `--smoke-test`, and `--demo-test` (AI auto-clear).

### Phase 7: Dual-Track Architecture & Bilingual Technical Documentation
- [x] **`01-top-down/` (Integrated View)**: Consolidated master compiler, linker, runtime, and applications with a 7-step automated master pipeline (`build-all.sh`).
- [x] **`02-down-top/` (Ladder View)**: Modeled the incremental 0-to-N philosophy through self-contained directories from `step00-seed` to `step05-applications`.
- [x] **Top-Level Bilingual Documentation**: Authored `README.md` and `README.zh-CN.md`, dissecting 9 system-level differences between macOS Darwin ARM64 and Windows PE32+ x64, with explicit model attribution to **Gemini 3.8 Flash (Medium)**.

### Phase 8: Native macOS Cocoa Interactive Game Window (`mario-extended`)
- [x] Built the interactive macOS Cocoa window version under `examples/mario-extended/` (completely isolated from the original headless test code).
- [x] Zero external dependencies, driven purely by C and Objective-C linking system-native `-framework Cocoa` with 60 FPS rendering and full keyboard controls.
- [x] Corrected vertical rendering orientation (HUD at top, Mario and ground at bottom, sky overhead).
- [x] Supports interactive keyboard play (`build.sh --run`) and live visual AI auto-play demo (`build.sh --demo`).
