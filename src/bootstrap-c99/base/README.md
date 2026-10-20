[简体中文](README.zh-CN.md) · English

# Base bootstrap toolchain

This directory contains `seed.hex`, the A0/A1/C0 assembly sources, the base C1 sources in C, and this stage's runtime and tests.

Its purpose is to obtain a base C1 from the seed that can compile later tools. These compilers and headers belong to intermediate stages; their capabilities and limits do not describe the final `host/c1-stage2.exe`. Use the final toolchain for applications; see [Usage](../../../USAGE.md).

- [Bootstrap design](../../../BOOTSTRAP.md): the relationship between A0, A1, C0, the bridge compilers, and base C1.
- [Walkthrough](../../../WALKTHROUGH.md): executable steps from Seed through C1.

`build.ps1` organizes the base build, `build-c1.ps1` builds the bridge compilers and verifies two self-compiled generations of base C1, and `audit.ps1` checks this stage's artifacts. Use the release-root entry points for the correct paths and environment configuration.

`SOURCES.json` lists the current build and test inputs.
