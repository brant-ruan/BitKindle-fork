[简体中文](WALKTHROUGH.zh-CN.md) · English

# Stage-by-stage build

This page provides stage commands. For design and dependencies, see [BOOTSTRAP.md](BOOTSTRAP.md).

Run steps 1–7 in order from the release root in **Windows PowerShell 5.1**, proceeding only after each step succeeds. Artifacts are placed in `work/walkthrough/`; source paths such as `base/` and `host/` below are relative to `src/bootstrap-c99/`. For full acceptance testing, go directly to step 8 without first taking the staged route.

If execution policy blocks scripts, start a new session with `powershell.exe -NoProfile -ExecutionPolicy Bypass`, then run these commands. This affects only that session, not persistent execution policy.

## Bootstrap baselines

The stage scripts compare bootstrap artifacts and check the following fixed SHA256 values for the A0 seed and generation two of final C1. Other generations are checked by equality comparison. The linker only has a two-generation identity check, not a fixed hash check. Baselines do not replace functional tests.

| Artifact | SHA256 |
| --- | --- |
| A0 seed and both A0 generations | `BA0F84742B07A4A5340D0641B676EE18269943191139B570667D9984A98465E5` |
| Both final C1 generations | `DB92844256433C7020D0C399E8F32D9CB9BE4954CC4063EAC0DFEAD0A4570ADD` |

## 1. Decode the seed

Input: `src/bootstrap-c99/base/seed.hex`, the literal hexadecimal text of a complete PE program. Comments and whitespace are not part of its bytes.

```powershell
./build-stage.ps1 -Stage Seed
```

The script invokes system PowerShell 5.1, removes comments, validates hexadecimal byte pairs, and uses `WriteAllBytes` to write the 3,584-byte `work/walkthrough/base/a0-seed.exe`. The decoder does not resolve labels or generate instructions or PE structures.

Success requires `PASS walkthrough stage Seed` and the seed SHA256 listed above.

The decoder can also be run separately:

```powershell
New-Item -ItemType Directory -Force work | Out-Null
& "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File ./src/bootstrap-c99/base/decode_seed.ps1 `
    -Source ./src/bootstrap-c99/base/seed.hex -Output ./work/a0-seed.exe
if ($LASTEXITCODE -ne 0) { throw 'Seed decoding failed' }
Get-FileHash ./work/a0-seed.exe -Algorithm SHA256
```

This writes `work/a0-seed.exe` for standalone inspection. To continue this walkthrough, still run `-Stage Seed` to produce the artifact at the required path.

## 2. A0 assembles itself

```powershell
./build-stage.ps1 -Stage A0
```

Input: `base/a0.a0`. The seed produces `a0-stage1.exe`, and generation one produces `a0-stage2.exe`. All three hashes are compared, then A0 functional and error-handling tests run.

## 3. Build A1 with A0

```powershell
./build-stage.ps1 -Stage A1
```

Input: `base/a1.a0`. A0 produces `a1-stage0.exe`, then A1 produces itself twice. The three generations are checked for identity and A1 tests run.

## 4. Build C0 with A1

```powershell
./build-stage.ps1 -Stage C0
```

Input: `base/c0.a1`. A1 produces `c0.exe`, followed by C0 tests. C0 supports the C subset needed for bridging and outputs A1 assembly text.

## 5. Compile a C lexer with C0

```powershell
./build-stage.ps1 -Stage Lexer
```

Input: `base/c1/lexer.c`. Two C0 compilations must produce identical A1 text. A1 assembles `c1-lexer.exe`, which is tested. This stage retains reference data for the later self-compiled lexer.

## 6. Bridge to self-compiling base C1

```powershell
./build-stage.ps1 -Stage C1
```

This runs `base/build-c1.ps1`:

1. C0 and A1 compile `base/c1/bootstrap.c` into `c1-bridge.exe`.
2. The bridge compiles the capacity bridge source into `c1-capacity.exe`.
3. The capacity bridge compiles base C1 into `c1-stage0.exe`.
4. C1 generation zero → one → two, checking the latter two byte for byte.
5. Custom tools rebuild the mathematical table, run both generations' functional tests, and rebuild the lexer for comparison.

Log: `work/walkthrough/C1.log`. Its end should include success markers for self-compilation and both generations' functional tests.

## 7. Final toolchain

```powershell
./build-stage.ps1 -Stage Toolchain
```

`build-toolchain.ps1` builds the identifier-extended compiler, object-output compiler, custom linker, then `host/c1-stage0.exe`, `host/c1-stage1.exe`, and `host/c1-stage2.exe`. The runtime is rebuilt for each compiler generation. Compiler and runtime objects from the final two generations are compared byte for byte, followed by integration tests.

Final tools: `work/walkthrough/host/c1-stage2.exe` and `work/walkthrough/link-stage2.exe`. After `PASS walkthrough stage Toolchain`, build and run an example:

```powershell
./src/examples/minicurl/build.ps1 -BuildDirectory ./work/walkthrough
if (!$?) { throw 'Example compilation failed' }
./src/examples/minicurl/minicurl.exe --help
./src/examples/minicurl/test.ps1
```

## 8. Full release regressions and audit

The staged route does not include all final language/library regressions and supplemental acceptance tests. Use:

```powershell
./build-all.ps1 -Full
```

This rebuilds from the seed into `src/bootstrap-c99/build-from-seed/`, without reusing walkthrough artifacts. Both that directory and `work/release-run/` must not exist; see [README.md](README.md#cleaning-and-rebuilding) for rebuilding. Each final compiler generation must pass 4,992 regressions, plus the overall audit, three supplemental packages, and example checks listed in the README. Results are in `work/release-run/RESULT.json`.

## Progress and failures

In another PowerShell window, select the log for the current build route:

```powershell
# Staged C1 build
Get-Content ./work/walkthrough/C1.log -Tail 20
# Toolchain build in default or -All mode
Get-Content ./work/release-run/01-fast-build.txt -Tail 20
# Base tool build in -Full mode
Get-Content ./src/bootstrap-c99/build-from-seed/BASE-VERIFICATION.txt -Tail 20
```

Stage scripts do not overwrite existing stage logs. On failure, inspect and preserve the log first. Before retrying, use `clean-all.ps1` to remove default and staged outputs (scope in README), or use an unbuilt release copy. Do not fill missing outputs with old EXEs.

Rejection tests may intentionally print `C1 error`. Judge success by script exit status and the final PASS marker, not by an EXE appearing or a single diagnostic.

## Appendix: custom build directories and separate audits

These commands run the build, overall audit, and three supplemental suites separately, using fresh build and test directories. They do not include the hello, argument, and minicurl acceptance checks in `-Full`.

```powershell
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force work | Out-Null
$Snapshot='src/bootstrap-c99'
$Build='work/reproduce-build'
$Tests='work/reproduce-supplemental'
if(Test-Path $Build){throw 'Choose a build directory that does not exist'}
if(Test-Path $Tests){throw 'Choose a supplemental test directory that does not exist'}
foreach ($path in 'work/reproduce-implementation','work/reproduce-language') {
    if(Test-Path $path){throw "Choose a test directory that does not exist: $path"}
}

& "$Snapshot/build.ps1" -BuildDirectory $Build *> work/reproduce-build.log
if(!$?){throw 'Seed build failed'}

& "$Snapshot/audit.ps1" -BuildDirectory $Build -VerificationLog work/reproduce-build.log *> work/reproduce-audit.log
if(!$?){throw 'Full build audit failed'}

& src/c89-supplemental-tests/run.ps1 -Snapshot $Snapshot -BuildDirectory $Build -ScratchDirectory $Tests *> work/reproduce-supplemental.log
if(!$?){throw 'Supplemental tests failed'}

& src/c89-supplemental-tests/audit.ps1 -ScratchDirectory $Tests -VerificationLog work/reproduce-supplemental.log *> work/reproduce-supplemental-audit.log
if(!$?){throw 'Supplemental audit failed'}

& src/c89-implementation-tests/run.ps1 -Snapshot $Snapshot -BuildDirectory $Build -ScratchDirectory work/reproduce-implementation *> work/reproduce-implementation.log
if(!$?){throw 'Implementation probes failed'}

& src/c89-implementation-tests/audit.ps1 -ScratchDirectory work/reproduce-implementation -VerificationLog work/reproduce-implementation.log *> work/reproduce-implementation-audit.log
if(!$?){throw 'Implementation probe audit failed'}
& src/language-regression-tests/run.ps1 -Snapshot $Snapshot -BuildDirectory $Build -ScratchDirectory work/reproduce-language *> work/reproduce-language.log
if(!$?){throw 'Language regression probes failed'}

& src/language-regression-tests/audit.ps1 -ScratchDirectory work/reproduce-language -VerificationLog work/reproduce-language.log *> work/reproduce-language-audit.log
if(!$?){throw 'Language regression audit failed'}
```

Do not pass `-Smoke` to the overall audit. It checks completion markers, test results, and artifacts; supplemental suites also audit test sources and artifact equality. This route generates the C89/C99 runtimes needed by USAGE examples. When using [USAGE.md](USAGE.md), set `$build` in its initialization block to the absolute path of this build directory before setting `$compiler` and `$linker`. These custom directories and logs are outside `clean-all.ps1`'s scope.
