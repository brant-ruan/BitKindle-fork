# BitKindle macOS (Darwin ARM64) - Step-by-Step Bootstrap Ladder (02-down-top)

## Overview

While `01-top-down` provides the **high-level integrated system view**, `02-down-top` follows the incremental bottom-up bootstrapping path designed by original author tombkeeper: starting from a transparent hex seed and progressively building up the linker, compiler, runtime, and applications.

Each directory is a self-contained, individually verifiable milestone demonstrating the bottom-up progression.

---

## Ladder Directory Layout

```text
02-down-top/
├── build-all.sh                   # Master ladder runner traversing Step 00 through Step 05
│
├── step00-seed/                   # [Stage 0: Genesis]
│   ├── src/                       # seed-arm64.hex, seed_arm64.s
│   ├── decode_seed.sh             # Plain text to binary conversion tool
│   ├── run.sh                     # Stdin/stdout pipe echo test
│   ├── README.md & .zh-CN.md      # Stage notes
│
├── step01-linker/                 # [Stage 1: Foundation]
│   ├── src/                       # 64-bit Mach-O linker source (c1link-macho)
│   ├── test_fixture/              # Test object generator
│   ├── build_and_test.sh          # Symbol relocation, signing & execution tests
│   ├── README.md & .zh-CN.md      # Mach-O structures & AMFI signing
│
├── step02-compiler/               # [Stage 2: Compiler Backend]
│   ├── src/                       # c1-arm64 recursive descent compiler
│   ├── tests/                     # Loop, arithmetic & function call tests
│   ├── build_and_test.sh          # Compilation, .c1o generation, execution
│   ├── README.md & .zh-CN.md      # AAPCS64 frames & code emission
│
├── step03-runtime/                # [Stage 3: Interaction & Runtime]
│   ├── src/                       # Darwin runtime generator (build_runtime.c)
│   ├── tests/                     # String formatting & console I/O test (hello.c)
│   ├── build_and_test.sh          # Cross-module linking & console output
│   ├── README.md & .zh-CN.md      # Darwin svc #0x80 syscall conventions
│
├── step04-self-hosting/           # [Stage 4: Deterministic Closure]
│   ├── src/                       # bootstrap_core.c compiler core
│   ├── test_self_host.sh          # Gen0 -> Stage1 -> Stage2 SHA256 byte-for-byte check
│   ├── README.md & .zh-CN.md      # Byte-for-byte determinism proof
│
└── step05-applications/           # [Stage 5: Application Suite]
    ├── src/                       # hello, args, minicurl, mario (World 1-1)
    ├── test_all.sh                # Test suite for all 3 applications
    └── README.md & .zh-CN.md      # Application porting & execution guide
```

---

## Quick Start

Execute the master ladder pipeline from within this directory:

```bash
chmod +x build-all.sh
./build-all.sh
```

The pipeline systematically advances through and verifies Step 00 through Step 05!
