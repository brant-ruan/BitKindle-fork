[简体中文](C89-LIMITS.zh-CN.md) · English

# Translation resources and derived-declarator limits

This table describes translation resource limits enforced by `bootstrap-c99/host/compiler.c` and `host/object-output.c`. It records implementation boundaries; it does not imply that all capacities can be exhausted simultaneously in one program or replace minimum-capacity tests required by the standard. Predefined symbols, generated code, alignment, and temporaries may consume shared budgets.

| Resource | Limit or rejection condition | Source entry point |
| --- | --- | --- |
| Identifiers | 2048 internal entries, including predefined names; first 63 decoded characters significant, case-sensitive; normalized UTF-8 spelling stored in a 256-byte slot per entry | intern |
| Ordinary pure pointer chains | At most 15 levels; one further derivation is rejected | make_pointer |
| Declarators | Recursion beyond 64 levels rejected; at most 255 nodes in one construction chain | declarator, decl_node |
| Array types | 1024 shared entries; fixed types reused by base type/length; runtime types also record captured bound and size locations | make_array, rebase_array |
| Fixed array size | Constant bound must be positive and at most `0x1a00000`; fixed element size times bound must not exceed the same byte limit | runtime_array_bound, make_array |
| Function types | 1024 distinct signatures | make_function |
| Function parameters | At most 127 named/fixed parameters; variadic tails have separate encoding checks | parameter, function_suffix |
| Qualified types | 2048 distinct base-type/qualifier combinations | qualify_type |
| Struct/union types and enum types | 1024 type entries each | record_specifier, enum_specifier |
| Struct/union member records | 8191 usable records in a shared member pool | record_specifier |
| Struct/union nesting | More than 64 levels rejected | record_specifier |
| Active local bindings | 256, including parameters, typedefs, and local extern bindings | local and declaration handlers |
| Fixed stack frame | Parameter saves, fixed local objects, runtime-bound slots, and temporaries share 65536 bytes; locals charged at 8-byte boundaries, temporaries aligned separately; frame_peak records maximum active usage | local, temporary, function |
| VLA element storage | Runtime stack allocation, 16-byte aligned with page probing; each local VLA also uses a 16-byte descriptor and related bound slots in the fixed frame; elements do not count toward the fixed-frame 65536-byte limit | allocate_runtime_array, extent_slot, local |
| Variably modified type scope records | At most 4095 per function; prototype-only processing does not create these records | bind_variable_type |
| goto records | At most 4096 per function; scope checks and stack restoration resolved once labels are known | record_vm_goto, resolve_vm_gotos |
| Static-data allocation cursor | Must not exceed `0x2000000`; alignment and existing allocations consume the budget | allocate_global, static_declaration |
| Expression, unary-expression, and statement recursion | More than 128 levels rejected for each | expression, unary, statement |
| Integer/address constant-expression recursion | More than 128 levels rejected for each | ce_unary, ce_inner, sc_unary, sc_expression |
| Initializer recursion | More than 64 levels rejected; extra braces around bit-fields count toward this limit | initialize_object, initialize_bitfield |
| Initializer checkpoints | At most 128 nested frames; additional frames also require successful heap allocation | initializer_checkpoint_push |
| Active switch cases | 2048 records; nested switches share the current pool | switch_label |
| Call, object-reference, and address-relocation records | Each front-end pool holds 16384; total combined object relocations must also not exceed 16384 | invoke, object_reference, address-reference handlers, object-output.c |
| String-content pool | 1048576 content units shared by wide and narrow strings | string_token |
| Generated image/object output | Image cursor must stay below 1049088; total object size must not exceed 1049088, including header and symbol/relocation information | emit, object-output.c |
| Source input and preprocessor output | Each limited by a 1048576-unit buffer; generated preprocessor spellings also share 1049088 units | main, pp_put, pp_generate |
| Macro names | 1024 allocated entries; undef does not reclaim name entries | pp_define |
| Macro parameters | At most 127 ordinary parameters; variadic slots and invocation arguments have additional checks | pp_define, macro argument collection |
| Macro-definition token pool | 65536 entries | pp_save_token |
| Expansion token nodes | 65535 usable nodes, reusable after release | pp_node |
| Hide-set records | 131071 usable entries; expansion reset/reuse rules also apply | hide-set allocation functions |
| Macro expansion recursion and conditional inclusion nesting | At most 128 levels each | pp_expand_stream, pp_directive |
| Include depth | Root depth is 0; further inclusion is rejected at current depth 63 | pp_load_header |
| Header path | At most 1023 bytes plus terminator | pp_header_char, pp_open_candidate |
| C1_INCLUDE environment string | 4096-byte buffer; rejected if the interface reports a length of 4096 or more | pp_search_header |

An ordinary pure pointer chain is not the total depth of every mixed derived declarator. Arrays, functions, and qualified types create new type nodes and remain subject to declarator node counts, recursion depth, and the relevant type-table capacities. The 15-level limit is not a universal limit for mixed declarators, and a recursion limit of 64 does not promise 64 consecutive pointers.

After an initializer expression has been written to its target, the compiler restores that expression's temporary-allocation cursor unless it created a compound literal that must survive until block exit. Maximum frame usage remains recorded. Thus successive initializers do not accumulate storage for expired temporaries. Compound-literal storage is retained for its lifetime and cannot be reused early by restoring the cursor. `complex/test-initializer-lifetime.ps1` checks this lifetime and storage-reuse behavior.

VLAs and variably modified types are C99 features. Their entries describe consumption of the compiler's shared resources, not C89 syntax requirements. Runtime stack capacity also depends on the operating system and image configuration; these limits do not guarantee allocation of arbitrarily sized dynamic arrays.

The combined limits test in `c89-supplemental-tests` uses both compiler generations to check C89 minimum combined capacities, including 511 macros and file-scope objects, 127 local names/members/enumerators, 257 cases, 31 function/macro parameters, and 8 levels of include/conditional nesting. It does not test every limit above or prove compliance with all higher C99 minimum capacities.

This table covers compiler translation resources. The standalone linker, available operating-system memory, file lengths, and process stack impose additional limits. They are not combined into a single purported maximum program size.
