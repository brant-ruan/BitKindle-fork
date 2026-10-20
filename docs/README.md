[简体中文](README.zh-CN.md) · English

# Technical documentation

This directory covers implementation design, the object file format, and mathematical algorithms. Language behavior and platform conventions are documented in [Implementation notes](../src/C89-IMPLEMENTATION.md); resource limits are in [Limits](../src/C89-LIMITS.md).

| Document | Contents |
| --- | --- |
| [Implementation design](DESIGN.md) | Initializer state preservation, header/runtime isolation, bootstrap runtime, and test organization |
| [C1O1 object format](C1O-FORMAT.md) | File layout, section and symbol records, relocation calculations, and validation constraints |
| [Cube root](algorithms/CBRT.md) | Integer cube root, exact midpoint comparison, and directed rounding |
| [Fused multiply-add](algorithms/FMA.md) | Exact integer-domain multiply-add with a single final rounding |
| [Error functions](algorithms/ERF.md) | Piecewise erf/erfc algorithms, independent reference values, and accuracy limits |
| [Gamma functions](algorithms/GAMMA.md) | Recurrence, reflection, asymptotic expansion, and validation limits |

Suggested reading order: [Project overview](../README.md) → [Bootstrap design](../BOOTSTRAP.md) → [Usage](../USAGE.md). For build instructions, see the [Walkthrough](../WALKTHROUGH.md).
