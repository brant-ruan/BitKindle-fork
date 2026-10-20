[简体中文](README.zh-CN.md) · English

# C89 supplemental verification sources

These source-only tests target an already self-built compiler/linker/runtime.
They supplement, rather than replace, the toolchain's seed build and audit.
No executable or object is supplied or copied. The runner materializes only
source text into a fresh scratch directory and uses
the selected custom tools for every binary output.

From the release root after the selected snapshot build finishes:

```powershell
& src/c89-supplemental-tests/run.ps1 -Snapshot src/bootstrap-c99 -BuildDirectory src/bootstrap-c99/build-from-seed -ScratchDirectory work/supplemental-seed-check *> work/supplemental-seed-check.log
```

Default coverage: 210 ordered header pairs and 30 complete header orders,
recursive macros/stringification/pasting in executable and object modes,
256-byte C-locale conversion and boundaries, record sorting/searching, and
the documented implementation-fact probe. Both compiler generations are used.
Headers are not all possible complete permutations. These tests do not by
themselves establish complete C89 conformance. -Cases permits an explicitly
reported subset for focused development verification.
After the runner completes, independently verify the materialized sources,
execution gates and all 276 nonempty artifact pairs:

```powershell
& src/c89-supplemental-tests/audit.ps1 -ScratchDirectory work/supplemental-seed-check -VerificationLog work/supplemental-seed-check.log
```

The read-only audit does not replace the compiler/runtime seed provenance audit.
Additional coverage includes 69,634 integer narrowing inputs and six static initializers per compiler generation in direct and object modes, plus a combined C89 minimum translation-limit probe. Narrowing outcomes test this implementation's documented choices, not universal C requirements.
