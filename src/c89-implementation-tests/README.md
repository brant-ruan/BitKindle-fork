[简体中文](README.zh-CN.md) · English

# Implementation behavior probes

Six source-only suites cover character mappings, record layout, pointer/integer
conversion, default rounding, mathematical errors and signal dispositions.
They document implementation choices; passing is not complete C89 conformance.
No executable or object is supplied or copied. All binary output comes from
the selected self-built compiler/linker. Run from the release root:

```powershell
$ErrorActionPreference='Stop'
& src/c89-implementation-tests/run.ps1 -Snapshot src/bootstrap-c99 -BuildDirectory src/bootstrap-c99/build-from-seed -ScratchDirectory work/implementation-reproduce *> work/implementation-reproduce.log
if(!$?){throw 'Implementation probes failed'}
& src/c89-implementation-tests/audit.ps1 -ScratchDirectory work/implementation-reproduce -VerificationLog work/implementation-reproduce.log
if(!$?){throw 'Implementation probe audit failed'}
```

Use a fresh scratch directory. Both compiler generations execute every suite.
The audit compares materialized source text with current templates and checks
60 execution gates and 56 equal nonempty pairs.
The layout direct probe contains its own clearing loop; the linked variant
uses the C89 runtime. Signal probes observe child process exit codes.
