# Stage 0: Pure Hexadecimal ARM64 Machine Code Seed (Step 00 - Seed)

## Overview

The core philosophy of self-hosting is the **minimization and transparency of the root of trust**. This project does not rely on opaque, pre-compiled compiler binaries. Instead, it sprouts from a transparent text file of pure hexadecimal bytes (`seed-arm64.hex`).

On Apple Silicon (macOS Darwin ARM64), this seed file encapsulates a minimal 64-bit Mach-O executable capable of reading from standard input and echoing to standard output.

---

## File Structure

- `src/seed-arm64.hex`: Pure hex machine code seed file (aligned to 16KB page boundaries with self-contained headers);
- `src/seed_arm64.s`: Assembly mnemonic source code with detailed comments on the Mach-O header, load commands, and syscall dispatch;
- `decode_seed.sh`: POSIX Shell decoding script that converts hex text into an executable binary and applies an ad-hoc code signature;
- `run.sh`: Automated build and I/O pipeline verification test harness.

---

## Architectural Principles

1. **64-bit Mach-O Layout**:
   - `MH_MAGIC_64` (`0xfeedfacf`) identifies 64-bit Mach-O binaries;
   - `CPU_TYPE_ARM64` (`0x0100000c`) targets the AArch64 architecture;
   - Load commands define `LC_SEGMENT_64` (`__PAGEZERO` and `__TEXT`) and `LC_UNIXTHREAD` to initialize the program counter (`pc`).
2. **16KB Page Alignment**:
   - Apple Silicon hardware strictly enforces 16KB (`0x4000`) physical alignment for memory mappings;
   - While effective code is only ~140 bytes, the remainder is padded with zeroes to comply with hardware page boundaries.
3. **Darwin Raw Syscalls**:
   - `x16 = 3`: `sys_read(0, buf, 1024)` reads from standard input;
   - `x16 = 4`: `sys_write(1, buf, len)` writes to standard output;
   - `x16 = 1`: `sys_exit(0)` terminates cleanly;
   - Dispatched directly via `svc #0x80`.

---

## Verification

```bash
chmod +x run.sh decode_seed.sh
./run.sh
```
