# Stage 1: Standalone 64-bit Mach-O Linker & Code Signing (Step 01 - Linker)

## Overview

Before creating a compiler from scratch, **one must first construct a linker capable of assembling object files into valid executables**.

The original BitKindle linker targeted Windows PE32+ (x64). In this stage, the linker backend is completely rewritten into `c1link-macho`, an independent 64-bit Mach-O linker purpose-built for Apple Silicon.

---

## File Structure

- `src/`: Linker core implementation:
  - `link.c`: Linker CLI entry point, startup stub synthesizer, and two-pass linker driver;
  - `macho.c`: 64-bit Mach-O segment/section tables and load command emission;
  - `relocate.c`: ARM64 instruction relocation patches (`PAGE21`, `PAGEOFF12`, `BRANCH26`);
  - `merge.c`: Multi-object (`.c1o`) section merging algorithms;
  - `symbols.c`: Global/local symbol export, resolution, and collision binding;
  - `sections.c`: Memory layout & 16KB physical page alignment computation;
  - `fixups.c`: Reference fixup calculation;
  - `object.c` / `writer.c` / `files.c`: Object file parsing and serialization;
- `test_fixture/`: Mock `.c1o` and Mach-O prototypes;
- `build_and_test.sh`: Automated compilation, test object emission, Mach-O linking, and execution script.

---

## Technical Highlights

1. **AAPCS64 Relocation Types**:
   - `LK_ARM64_BRANCH26`: 26-bit relative branch/call relocation (`b`, `bl`);
   - `LK_ARM64_PAGE21`: `adrp` page-relative offset computation (`immhi:immlo`);
   - `LK_ARM64_PAGEOFF12`: 12-bit intra-page offset extraction for `add` / `ldr`.
2. **Apple Kernel AMFI & Ad-hoc Signing**:
   - Apple Silicon kernels enforce strict Apple Mobile File Integrity (AMFI); unsigned Mach-O binaries fail immediately with `SIGKILL`;
   - `c1link-macho` formats appropriate header padding and invokes `codesign -s - --force` automatically upon writing.

---

## Verification

```bash
chmod +x build_and_test.sh
./build_and_test.sh
```
