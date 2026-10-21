# Stage 2: Native ARM64 C1 Recursive Descent Compiler Backend (Step 02 - Compiler)

## Overview

With the Mach-O linker in place from Stage 1, we proceed to the **compiler**.

`c1-arm64` translates the C1 language subset directly into native ARM64 (AArch64) machine instructions, incorporating a lexer, recursive descent parser, AAPCS64 frame layout manager, virtual register allocator, and binary emitter generating `.c1o` object files.

---

## File Structure

- `src/c1_arm64.c`: Complete single-file self-contained compiler implementation;
- `tests/test_calc.c`: Integration test source featuring arithmetic, nested loops, multi-argument function calls, and conditional branches;
- `build_and_test.sh`: One-click script for compiler bootstrap, C source compilation, object linking, and exit code validation.

---

## Technical Highlights

1. **AAPCS64 Frame & Register Conventions**:
   - Manages stack frames via `stp x29, x30, [sp, #-16]!` and `mov x29, sp`;
   - Argument passing uses `x0` through `x7`;
   - Stack allocations strictly adhere to 16-byte alignment;
   - Expression evaluation centers on `x0` as the primary accumulator.
2. **Conditional Branches & Inverted Encodings**:
   - ARM64 `cset Rd, cond` maps fundamentally to `csinc Rd, zr, zr, invert(cond)`;
   - Inverted condition codes are handled precisely to maintain relational correctness.
3. **C Subset Support**:
   - Types: `int`, `char`, and pointer declarations (e.g. `char **argv`);
   - Control flow: `if` / `else`, `while` loops, `return`;
   - Expressions: Binary operators (`+`, `-`, `*`, `/`, `%`), comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), and function calls.

---

## Verification

```bash
chmod +x build_and_test.sh
./build_and_test.sh
```
