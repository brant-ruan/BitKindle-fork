[简体中文](C1O-FORMAT.zh-CN.md) · English

# C1O1 object file format

`.c1o` is this toolchain's private format for separate compilation and linking. Its current signature is `C1O1`, and its target is Windows x64. It is not a COFF `.obj`, a PE executable, or an import library.

This page describes the current implementation. Reading and validation follow [linker/object.c](../src/bootstrap-c99/linker/object.c); the final compiler's writer is in [host/object-output.c](../src/bootstrap-c99/host/object-output.c). Files are serialized with explicit byte operations, not by dumping C structures. There is no shared format header.

## Encoding and overall layout

All multibyte integers are little-endian. Below, `u32` and `u64` denote unsigned 32- and 64-bit integers; `i64` denotes a signed two's-complement 64-bit integer. All indices are zero-based. Field offsets in tables are relative to the start of the record.

Regions appear in this order, with no implicit padding:

```text
32-byte file header
Section table:    section_count × 48 bytes
Symbol table:     symbol_count × 40 bytes
Relocation table: fixup_count × 32 bytes
Name area:        name_size bytes
Payload area:     payload_size bytes
```

The file offsets are therefore:

```text
section_offset = 32
symbol_offset  = section_offset + section_count * 48
fixup_offset   = symbol_offset  + symbol_count * 40
name_offset    = fixup_offset   + fixup_count * 32
payload_offset = name_offset   + name_size
file_size      = payload_offset + payload_size
```

The actual file length must equal `file_size` exactly. Truncation and trailing data are rejected.

## File header: 32 bytes

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | 4 bytes | ASCII `C1O1`, or `43 31 4F 31` |
| 4 | u32 | Machine type; must be `0x8664` |
| 8 | u32 | `section_count`: number of section records |
| 12 | u32 | `symbol_count`: number of symbol records |
| 16 | u32 | `fixup_count`: number of relocation records |
| 20 | u32 | `name_size`: size of the name area in bytes |
| 24 | u32 | `payload_size`: size of the payload area in bytes |
| 28 | u32 | Reserved; must be zero |

## Section record: 48 bytes

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u64 | In-memory size `memory_size` |
| 8 | u64 | File-data size `file_size` |
| 16 | u64 | Memory alignment `alignment` |
| 24 | u64 | File alignment required in the output layout, `file_alignment` |
| 32 | u64 | Offset of this section's data from the start of the payload area |
| 40 | u32 | Attribute bits: read = 1, write = 2, execute = 4 |
| 44 | u32 | Reserved; must be zero |

Constraints:

- `file_size <= memory_size`; both alignments must be nonzero powers of two.
- Attributes must be in 1–7: for example, read/write is 3 and read/execute is 5.
- Sections with file data must occupy the payload contiguously in section-table order, with no gaps, overlaps, or unused trailing data.
- When `file_size == 0`, the data offset must be zero. Such sections occupy memory only, as in BSS. Memory beyond a section's file-data size is zero-filled.

Section records have no section names. The file-alignment requirement does not imply alignment padding inside the `.c1o` payload.

## Symbol record: 40 bytes

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | Name offset from the start of the name area |
| 4 | u32 | Binding type; see below |
| 8 | u32 | Section index, or `0xFFFFFFFF` if none |
| 12 | u32 | Reserved; must be zero |
| 16 | u64 | Symbol offset `value` from the start of its section |
| 24 | u64 | Symbol `size` |
| 32 | u64 | Required `alignment` |

| Value | Name | Semantics |
| --- | --- | --- |
| 0 | `LK_UNDEFINED` | Undefined reference to be resolved at link time |
| 1 | `LK_LOCAL` | Definition local to the current input module |
| 2 | `LK_GLOBAL` | Global definition available to other modules |
| 3 | `LK_COMMON` | Common zero-initialized storage allocated by the linker |

Names are nonempty NUL-terminated byte strings, 1–255 bytes long excluding the terminator. They are packed contiguously in symbol-table order; identical names still have separate copies. No unused trailing bytes are allowed in the name area. The reader treats names as bytes and performs no Unicode conversion.

Every symbol's alignment must be a nonzero power of two. Local and global definitions require valid section indices, with `value` and `size` within the section's memory range. Undefined and COMMON symbols require section index `0xFFFFFFFF` and `value` zero.

Duplicate global definitions are link errors. A global definition takes precedence over COMMON of the same name. COMMON symbols with the same name are merged using the maximum size and alignment. Undefined symbols may remain unresolved temporarily, but must resolve when their address is needed. See [symbols.c](../src/bootstrap-c99/linker/symbols.c).

## Relocation record: 32 bytes

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u32 | Index of the section containing the patch field |
| 4 | u32 | Target symbol index |
| 8 | u32 | Relocation type |
| 12 | u32 | Reserved; must be zero |
| 16 | u64 | Patch-field offset from the start of its section |
| 24 | i64 | Explicit addend `A` |

Let `S` be the resolved target symbol's virtual address, `P` the address of the first byte of the patch field, and `B` the image base. The linker computes the value below and overwrites the field in little-endian order. The original field is not an implicit addend.

| Value | Type | Bytes | Value written |
| --- | --- | --- | --- |
| 1 | `LK_ABS64` | 8 | `S + A`, within unsigned 64-bit range |
| 2 | `LK_ABS32` | 4 | `S + A`, within unsigned 32-bit range |
| 3 | `LK_REL32` | 4 | `S + A - (P + 4)`, within signed 32-bit range |
| 4 | `LK_RVA32` | 4 | `S + A - B`, within unsigned 32-bit range |

All types first check `S + A` for unsigned 64-bit overflow or underflow. Section and symbol indices must be valid. The entire patch field must lie within the section's file data, not merely its zero-filled region. Patch fields in one section must not overlap; this is checked when applying relocations.

For example, `LK_REL32` with `S=0x401100`, `P=0x401020`, and `A=0` writes `0xDC`, or bytes `DC 00 00 00`.

Types and calculations are in [relocate.c](../src/bootstrap-c99/linker/relocate.c); batch validation and writing are in [fixups.c](../src/bootstrap-c99/linker/fixups.c).

## Current compiler output conventions

The format supports multiple sections and four relocation types. The final compiler currently emits a subset:

- Two sections: index 0 contains read/execute code and literal data; index 1 contains read/write zero-filled storage. Both use memory alignment 16 and file alignment 1.
- The first two symbols are local symbols `$code` and `$bss`, pointing to the starts of those sections.
- Function calls, object references, and address references use `LK_REL32`.
- When initialization code is needed, an additional local symbol `$init` is emitted. The linker recognizes it and calls the initialization code before the program entry point. This is a compiler/linker startup convention, not a file-header field.

DLL imports, the final entry symbol, and console/GUI subsystem selection are supplied on the linker's command line, not in the `.c1o` header. See [Usage](../USAGE.md).

## Implementation capacities and validation boundaries

The reader imposes the following per-object limits. They are implementation capacities, not the limits of field widths:

| Item | Limit |
| --- | ---: |
| Sections | 1,024 |
| Symbols | 4,096 |
| Relocations | 16,384 |
| Name area | 1 MiB |
| Payload area | 16 MiB |

Merged sections, symbols, and relocations are also subject to corresponding fixed capacities; every input cannot simultaneously consume all of them. The final compiler separately limits total object output to 1,049,088 bytes, so the reader accepts a larger range than the compiler can currently produce.

Passing structural validation does not guarantee successful linking. The linker also checks symbol conflicts, unresolved references, overlapping patch fields, address and relative-displacement overflow, and PE layout constraints.
