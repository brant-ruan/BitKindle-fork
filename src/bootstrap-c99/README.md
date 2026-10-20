[简体中文](README.zh-CN.md) · English

# BitKindle bootstrap toolchain

This directory contains the sources and tests needed to build C1, its runtime, and the PE linker from a hexadecimal seed. This page is a source guide. See the [release overview](../../README.md) for environment and build modes, [Bootstrap design](../../BOOTSTRAP.md) for dependencies, and [Implementation notes](../C89-IMPLEMENTATION.md) and [Limits](../C89-LIMITS.md) for language/library boundaries.

## Build and audit entry points

Script paths below are relative to this directory. Invoke them from the release root to satisfy source-relative include paths.

| Entry point | Responsibility |
| --- | --- |
| Release-root `build-all.ps1` | Recommended: default fast build, `-All` adds example compilation, `-Full` runs full release acceptance checks; see the release overview |
| `build.ps1` | Builds from the seed, runs the base-stage audit, integration and language/library tests, and both final compiler generations' regressions; defaults to `build/` here |
| `audit.ps1` | Checks build logs, results, and artifacts. For separate invocation, supply matching `-BuildDirectory` and `-VerificationLog`; do not use `-Smoke` for full audit |
| `build-toolchain.ps1` | Builds later stages from base C1 in `-BaseBuild`, writing to `-BuildDirectory`; runs integration tests by default. `-SkipTests` skips them but retains self-compilation identity checks |
| `tools/build-fast.ps1` | Used by default release builds: skips regression/integration tests and builds the toolchain and C89/C99 application runtimes |
| `compile-program.ps1` | Compiles one application source, selects its runtime, and invokes the linker; see [Usage](../../USAGE.md) |

Local `build.ps1` is not the full release acceptance route: root `build-all.ps1 -Full` orchestrates the overall audit, three supplemental suites, and example checks. Staged and custom-directory commands are in [WALKTHROUGH.md](../../WALKTHROUGH.md).

## Source directories

The table covers all 15 top-level source directories. Earlier compilers, components, and tests are build or validation inputs, not generated artifacts.

| Directory | Purpose |
| --- | --- |
| `base/` | Seed, A0/A1, C0, bridges, and base C1, with headers, base libraries, build scripts, and tests |
| `identifier/` | Identifier-extended C1, built by base C1 and then self-compiled |
| `object/` | C1 with custom `.c1o` output; builds the linker and objects for final C1 generation zero |
| `host/` | Final C1's `compiler.c` and `object-output.c`, compiled separately from the runtime and linked |
| `linker/` | Reads `.c1o`, merges sections, resolves symbols/relocations, and emits DLL imports and Windows x64 PE with console or GUI subsystem |
| `runtime/` | Full, C89, and initial bootstrap runtime entry points, Windows import list `imports.json`, and base implementations in `lib/` |
| `components/` | Runtime components organized by function, accompanying headers, and import declarations |
| `complex/` | Complex and real mathematical extensions, type-generic headers, tests, and reference data |
| `include/` | Public standard-library and internal headers; drivers also add selected component directories and `base/include/` |
| `features/` | Language/library test entry points; `library/` organizes library tests, `run-full.ps1` runs final-compiler regressions |
| `diagnostics/` | Invalid-input, preprocessing, and language-mode diagnostic tests and audits |
| `namespace/` | Public headers, symbol namespaces, C89/C99 interface conventions, and locale/numeric/encoding/VLA boundary tests; generates the C89 application runtime |
| `integration/` | Multiple-object linking, runtime, DLL-import, and PE-subsystem integration tests |
| `examples/` | Minimal hello and argument examples; minicurl and Mario are in release `src/examples/` |
| `tools/` | Compilation processes and binary-stream transport, fast builds, source-path checks, and artifact audits |

## Sources in the final EXEs

`build-toolchain.ps1` organizes the final compiler build:

```text
host/compiler.c (includes host/object-output.c) → compiler2.c1o ─┐
runtime/runtime.c → runtime2.c1o ────────────────────────────────┼→ host/c1-stage2.exe
runtime/imports.json (Windows DLL import configuration) ────────┘
```

The full runtime includes base-library and wide-character components through `components/wide-input/runtime.c` → `wide-output/runtime.c` → `wide-io/runtime.c`, plus `complex/lib/all.c` and `complex/lib/real-extra.c`. Thus the final compiler's implementation extends beyond `host/`.

Generation zero uses `runtime/bootstrap-runtime.c`; the next two use `runtime/runtime.c`. `runtime/runtime-c89.c` selects C89 mathematical, numeric, and locale policies for applications. Each entry includes only required files; do not assume all of `runtime/lib/` is compiled in.

The final linker is built by object-stage C1 compiling `linker/link.c` directly to EXE. That entry includes other `linker/` implementations and `base/lib/memory.c`, `string.c`, `errno.c`, and `stdio.c`; the latter also includes `process.c`. It uses `base/include/` and startup structures from direct PE output. It does not separately link the final compiler's `runtime2.c1o`, or include implementations from `host/`, `components/`, or `complex/`.

`identifier/` and `object/` provide preceding tools; their compiler sources are not directly incorporated into the final EXEs. Tests are not compiled into the final tools either. Actual dependencies follow entry-point includes and build scripts.

## components subdirectories

Components are composed through source includes and need not be compiled individually. `object-runtime/` supplies import declarations; the others contribute implementations through the full runtime to final C1. They are not part of the final linker's implementation.

| Subdirectory | Purpose |
| --- | --- |
| `object-runtime/` | Windows import-symbol declarations referenced by the compiler and runtime |
| `wide-io/` | Base runtime, startup, and wide-character file I/O; `lib/` also contains memory, strings, ordinary stdio, locale, signals, and other base implementations |
| `wide-output/` | Wide formatted output and decimal/hexadecimal numeric formatting, extending wide-io |
| `wide-input/` | Wide formatted input and floating scanning, extending wide-output; its runtime.c is the component entry used by the final runtime |
| `wchar-string/` | Wide-string operations and declarations |
| `restartable/` | Stateful restartable multibyte/wide-character conversion |
| `wide-integer/` | Wide-string-to-integer conversion |
| `wide-float/` | Wide-string-to-floating conversion and numeric-conversion core |
| `wide-time/` | Wide date/time formatting |
| `wctype/` | Wide-character classification, case conversion, and descriptor interfaces |
| `inttypes/` | Maximum-width integer interfaces, numeric conversions, and `<inttypes.h>` declarations |

## complex implementation and tests

- `include/`: complex, type-generic mathematical, and related headers.
- `lib/`: complex arithmetic, complex functions, and real mathematical extensions; `all.c` and `real-extra.c` compose them into the runtime.
- Top-level `*-client.c`, `*-reference.c`, and `*-cases.ps1`: functional tests, reference data, and case descriptions.
- `make-*-reference.ps1`: generates C reference-data text, not binary artifacts.
- `test-*.ps1`, `run.ps1`, `audit.ps1`, and `SUITES.json`: individual tests, suite orchestration, result auditing, and suite manifest.

See [Technical documentation](../../docs/README.md) for design and algorithms.

## Generated directories

The release entry writes to `build-from-seed/` here; direct `build.ps1` defaults to `build/`. Both contain generated tools, objects, and relevant logs.

Root `clean-all.ps1` removes `build-from-seed/` and other [default output locations](../../README.md#cleaning-and-rebuilding), not this directory's `build/` or custom locations. Retain source directories and `SOURCES.json`; it records build and test input paths without fixed file hashes, not a list of generated artifacts.
