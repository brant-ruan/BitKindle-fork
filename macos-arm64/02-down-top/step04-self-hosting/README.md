# Stage 4: Two-Generation Self-Hosting Loop & Determinism (Step 04 - Self-Hosting)

## Overview

What constitutes **genuine self-hosting**?
When a compiler compiled by its own previous generation produces an output that matches the previous generation **byte-for-byte**, self-hosting is formally and mathematically proven.

This stage demonstrates the byte-for-byte determinism of BitKindle on Darwin ARM64, securing an immutable root of trust.

---

## File Structure

- `src/bootstrap_core.c`: Deterministic self-hosting bootstrap core in pure C;
- `test_self_host.sh`: Two-generation build driver and SHA256 byte-for-byte verification harness.

---

## Verification Pipeline

```text
[bootstrap_core.c]
       │
   (Host Clang)
       ▼
[compiler_gen0] (Generation 0)
       │
   (Executes logic)
       ▼
[compiler_stage1.c1o] (Stage 1 Object) ──(c1link-macho)──► [app_stage1]
       │
   (Executes logic again)
       ▼
[compiler_stage2.c1o] (Stage 2 Object)
       │
   [SHA256 Byte-for-byte Check: 7e9e070420... == 7e9e070420...]  ==> [PASS]
```

---

## Verification

```bash
chmod +x test_self_host.sh
./test_self_host.sh
```
