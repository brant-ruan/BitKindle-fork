[简体中文](README.zh-CN.md) · English

# Language regression tests

This source-only package runs against two already self-built compiler generations. It covers 25 preprocessing expression cases in both C89/C99 modes, the C89 macro-rescanning example over 81 input pairs, and selected C99 floating-environment/pragma and conversion behavior. It supplements seed verification and does not certify complete C89/C99 or Annex F conformance.

From the release root, after building the selected snapshot:

```powershell
& src/language-regression-tests/run.ps1 -Snapshot src/bootstrap-c99 -BuildDirectory src/bootstrap-c99/build-from-seed -ScratchDirectory work/language-tests *> work/language-tests.txt
if(!$?){throw 'Language tests failed'}
& src/language-regression-tests/audit.ps1 -ScratchDirectory work/language-tests -VerificationLog work/language-tests.txt
```

Use a fresh scratch directory. The runner materializes only C/PowerShell text. All objects and executables come from the supplied custom compilers/linkers. The audit checks logs, rejection evidence, agreement between materialized source text and current templates, and 44 nonempty byte-identical artifact pairs. Preprocessing harness helpers are read from the supplied snapshot, whose source paths and build provenance must be checked separately.

Floating tests run in C99 mode; their inclusion does not turn C99 requirements into C89 requirements. FP_CONTRACT ON is permission to contract, not a requirement for a fused result.
