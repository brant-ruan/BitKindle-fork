[简体中文](C89-IMPLEMENTATION.zh-CN.md) · English

# C89/C99 implementation notes

This document describes language behavior, library conventions, and support boundaries for the Windows x64 implementation. Current sources are listed in bootstrap-c99/SOURCES.json. See [Usage](../USAGE.md) and the [Walkthrough](../WALKTHROUGH.md) for build and test commands.

Its coverage follows [C89 draft A.6.3](https://port70.net/~nsz/c/c89/c89-draft.html#A.6.3), which requires implementation-defined behavior to be documented. Self-compilation or selected tests cannot replace these conventions and their verification.

## Data model

| Type or property | Implementation choice | Evidence |
| --- | --- | --- |
| Byte | 8 bits | `include/limits.h`, facts probe |
| char / signed char | 1 byte, -128 to 127 | limits header, facts / integers probes |
| unsigned char | 1 byte, 0 to 255 | Same |
| short / unsigned short | 2 bytes; -32768 to 32767 and 0 to 65535 | Same |
| int / long | Both 4 bytes, -2147483648 to 2147483647 | Same |
| unsigned int / unsigned long | Both 4 bytes, 0 to 4294967295 | Same |
| Pointers | 8 bytes | facts probe |
| size_t / ptrdiff_t | unsigned long long / long long respectively, 8 bytes | `base/include/stddef.h`, facts probe |
| wchar_t | unsigned short, 2 bytes | Same, character probes |
| NULL | `((void *)0)` | `base/include/stddef.h` |
| Integer byte order | Little-endian | facts probe |
| float | 4 bytes, 24 binary significand bits | float header, facts probe |
| double / long double | Both 8 bytes, 53 binary significand bits | Same |

long long is a supported extension and defines the 64-bit size_t and ptrdiff_t types. Explicit C89 mode warns about long long syntax as an extension. This table does not claim that C89 itself specifies the type.

## Integer arithmetic and conversion

Runtime narrowing preserves the low bits of the destination width, then sign-extends or zero-extends according to the destination type. Signed results use two's complement. For example, `(signed char)255`, `(short)65535`, and `(int)4294967295U` are all -1. Signed right shift is arithmetic. Negative division examples are `-7/3 == -2` and `-7%3 == -1`.

Signed `&`, `|`, `^`, and `~` operate bitwise on two's-complement representations, not sign-magnitude values. Binary operators first perform integer promotions and usual arithmetic conversions; `~` complements the promoted type. Results are interpreted at the resulting integer width. Binary code generation in `host/compiler.c` uses AND/OR/XOR, unary generation uses NOT, and constant evaluation uses matching bit operations. Negative division truncates toward zero; a nonzero remainder has the dividend's sign.

For C89, this implementation defines signed left shift as shifting the promoted type's two's-complement bit pattern, discarding high bits, filling low bits with zero, and interpreting the result in that type. The count must be nonnegative and less than the type width. Thus `-1<<1` gives -2 and `1<<31` gives the minimum 32-bit signed value. Runtime and all three constant-evaluation paths use this rule. C99 retains its nonnegative-operand and representability restrictions. The choice follows [WG14 C90 DR 081](https://www.open-std.org/jtc1/sc22/wg14/issues/c90/issue0081.html).

Source evidence is in `convert`, `cast_to`, and integer code-generation paths in `host/compiler.c`. The supplemental integers suite checks 69,634 inputs and six static initializers per compiler generation through both direct-executable and separate-compilation/linking paths. This supports the stated choices but is not exhaustive over constant-folding combinations.

## Floating-point conversion

`floating_convert` uses SSE conversions for runtime integer/floating conversion and double/float precision changes. In the default environment, integer-to-floating conversion and floating narrowing use nearest, ties to even. Floating-to-integer conversion within representable range discards the fractional part toward zero. double and long double share one representation, so no extra precision conversion occurs. MXCSR controls runtime rounding. The public FLT_ROUNDS macro reads this state on each evaluation; it is not a fixed integer constant.

Conversion tests cover default-environment ties, signed values, fractional truncation, and selected static/runtime agreement cases. They do not prove all rounding modes, exceptions, or numerical boundaries.

## Mathematical error returns

The table lists error-return conventions for tested calls. Tests clear errno before each call group to distinguish new errors from existing state.

| Tested calls | Return value | errno |
| --- | --- | --- |
| acos(2), asin(-2), sqrt(-1), log(-1), log10(-1) | NaN | EDOM |
| pow(-2,0.5), fmod(1,0) | NaN | EDOM |
| log(0), log10(0) | -HUGE_VAL | ERANGE |
| pow(0,-1), exp(1000) | HUGE_VAL | ERANGE |
| exp(-1000), ldexp(DBL_MIN,-1000) | Zero | ERANGE |
| ldexp(DBL_MIN,-1) | Exact positive subnormal | Remains zero |
| pow(0,0) | 1 | Remains zero |
| atan2(0,0) | Zero | Remains zero |

`math-scale.c` sets ERANGE when the rounded result is tiny and inexact. An exactly scaled subnormal does not set errno for that reason; not every subnormal result represents reported underflow. Other functions are handled by math-asin, math-sqrt, math-log, math-pow, math-remainder, math-exp, and math-atan2. These are specific conventions verified by execution, not a proof for all arguments, rounding modes, or library-wide error bounds. No uniform worst-case error bound is provided.

## Pointer/integer conversion

Under `cast_to` and `convert` in `host/compiler.c`, pointers are 64-bit addresses. Explicit integer-to-pointer conversion sign-extends signed values of 32 bits or less according to their promoted type and zero-extends unsigned 32-bit values. A 64-bit integer's bit pattern is unchanged. Pointer-to-integer conversion keeps the low destination-width bits and interprets them in the target integer type. Address zero represents a null pointer.

Constructed pointers may be dereferenced or called only when the actual requirements of the target object or function are met. These round-trip conventions do not make arbitrary integers accessible addresses or guarantee portability to other C implementations.

## Characters and strings

Basic characters use ASCII values. Standard control escapes map to `\a=7`, `\b=8`, `\f=12`, `\n=10`, `\r=13`, `\t=9`, and `\v=11`. Escapes for backslash, single quote, double quote, and question mark retain the corresponding character value. Octal escapes consume at most three digits; hexadecimal escapes consume following hexadecimal digits. Narrow escapes range from 0..255 and wide escapes from 0..65535. Out-of-range or unknown escapes are diagnosed and rejected.

Narrow character constants have type int. Plain char is signed, so single-character constants with values 128..255 produce -128..-1; for example, `'\xff' == -1`. Narrow multicharacter constants concatenate bytes left to right and retain the low 32 bits: `'AB' == 0x4142`, `'ABCDE' == 0x42434445`. Wide character constants have type wchar_t and retain the last unit for multiple characters, for example `L'AB' == 66`.

Character and string handling is implemented in `escape`, `character`, and `string_token`.

Raw source bytes 128..255 independently map to character units of the same value; the int value of a narrow character constant additionally follows the sign rules above. The L prefix does not enable UTF-8 decoding. Source files use a single-byte mapping, not general Unicode source support, and this is distinct from header-path encoding.

C-locale multibyte conversion uses a stateless single-byte mapping: bytes 0..255 map to equal wide values; wide values above 255 cannot map back to a byte. Supplemental multibyte tests cover all 256 bytes, terminators, truncation, zero lengths, and invalid wide values. This is the implementation's locale choice.

## register and volatile

register does not require an object to remain in a machine register. The implementation still allocates stack locations for register locals and parameters, recording the attribute to diagnose disallowed operations such as taking their addresses. It is not a separate register-optimization mode. Source evidence includes parameter binding, `declaration`, `load`, and address operations.

Scalar accesses emit width-appropriate memory reads/writes through `load_opcode`, `load`, `indirect_value`, and `store`. volatile provides no additional atomicity or thread synchronization. Records containing volatile are materialized into temporaries by `record_read` when their entire value is consumed. Member selection, address formation, and assignment destinations do not first copy the whole record. `store_record` copies the complete record byte by byte, including padding. Volatile aggregate initialization prepares temporary content before copying it to the destination.

The C89 member-volatile, register-volatile, volatile-list, volatile-list-copy, volatile-list-helper, and related rejection tests cover these generation paths, not all device-memory, side-effect-counting, or concurrency scenarios.

## Structures, unions, enumerations, and bit-fields

Under `type_align` and `record_specifier`, scalar members align to their size, arrays use element alignment, and nested records use their own alignment. Ordinary struct members are placed sequentially after upward alignment; total size is rounded to the maximum member alignment. Union members all start at offset zero, with size equal to the largest member rounded to maximum alignment.

int bit-fields use 4-byte allocation units, low bits first. Consecutive fields share a unit if sufficient bits remain, otherwise starting in the next aligned unit; they never cross unit boundaries. Ordinary members and zero-width fields end the current bit-field unit. Plain int bit-fields are read as signed. Enums occupy 4 bytes and enumerator values have int range.

Reading a different union member reuses the underlying representation; writing `0x41424344` and reading a char member yields `0x44`. This does not promise usable values for arbitrary type combinations, and selected layout tests do not cover them all.

## Included files and pragmas

`#if` / `#elif` arithmetic uses signed or unsigned 64-bit integers, independently of the execution environment's 32-bit `long`. Thus `0xffffffffU + 1` does not wrap to zero in conditional inclusion, while `0xffffffffffffffffUL + 1` wraps modulo 2^64. Unsuffixed hexadecimal `0xffffffff` is a positive signed value here. C89 permits integer widths to differ between these environments; see [WG14 Rationale N937, section 5](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n937.pdf). C99 conditional inclusion uses the 64-bit domains corresponding to this implementation's intmax_t / uintmax_t. Tests are in `language-regression-tests/`.

Under `pp_search_header`, absolute paths are opened directly. Quoted includes search the current included file's directory, the process working directory, then the public search path. The public path searches nonempty semicolon-separated `C1_INCLUDE` directories in order, finally trying `include` under the working directory. Angle-bracket includes go straight to the public path. Paths are passed to Win32 `CreateFileA`; this does not promise additional non-ASCII filename compatibility.

Under `pp_pragma_apply`, non-STDC pragmas are ignored. STDC FP_CONTRACT, FENV_ACCESS, and CX_LIMITED_RANGE accept ON, OFF, and DEFAULT. FP_CONTRACT never contracts expressions and defaults to OFF. ON permits but does not require contraction, so operations remain separate. FENV_ACCESS defaults to OFF, and DEFAULT means OFF. In every state, the backend conservatively preserves ordered operations affected by the runtime floating environment; it does not use OFF to merge or reorder across calls. This permits ON regions to access rounding modes and status flags. Pragmas do not set or restore MXCSR; programs must meet prerequisites such as default control modes before entering OFF regions. Static constant evaluation uses default nearest-even rounding, separately from runtime controls. This is not exhaustive validation of every floating-library semantic. CX_LIMITED_RANGE defaults to OFF; every state retains robust complex multiply/divide implementations, without using ON to relax intermediate-range requirements. cx-pragma, multiply/divide range, and floating-environment tests check these policies. This conservative policy is not a claim of full optional Annex F/G conformance. FP_CONTRACT follows [N1256 6.5 and 7.12.2](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf).

## Character classification and locale

`components/wide-io/lib/ctype.c` restricts uppercase, lowercase, and digits to ASCII A..Z, a..z, and 0..9. isalpha is the union of letter sets; isalnum adds digits. Control characters are 0..31 and 127; printable characters are 32..126; graphical characters are 33..126; punctuation is graphical and nonalphanumeric. Whitespace is space and `\t\n\v\f\r`. Hexadecimal digits are 0..9, A..F, and a..f. Case conversion changes only these ASCII letters, returning other valid inputs, including EOF, unchanged. Bytes 128..255 belong to none of these classes.

`components/wide-io/lib/locale.c` provides only the C locale. For valid categories, `"C"` and the empty string select C; querying with a null pointer returns `"C"`. Other names return null. localeconv uses `"."` as decimal point, empty strings for other string fields, and CHAR_MAX for numeric convention fields. Runtime wrappers handle C89/C99 header member-name differences. Locale tests cover both layouts and name collisions.

## Signal handling

`base/include/signal.h` defines sig_atomic_t as int and supports SIGINT=2, SIGILL=4, SIGFPE=8, SIGSEGV=11, SIGTERM=15, and SIGABRT=22. `components/wide-io/lib/signal.c` initializes every disposition to SIG_DFL. signal returns the previous handler. Invalid signal numbers or SIG_ERR as a new handler return SIG_ERR and set errno to EINVAL.

raise invokes the handler synchronously in the calling thread. SIG_IGN succeeds immediately. Before calling a custom handler, the disposition is reset to SIG_DFL. Default handling terminates immediately, with exit code 3 for SIGABRT and 128 plus the signal number otherwise. abort terminates with status 3 even if SIGABRT is ignored or its handler returns. The module uses an internal handler table; it does not promise conversion of Windows console events or hardware exceptions into C signals.

Signal probes cover default, ignore, handler-reset, and abort behavior. Their results do not guarantee all combinations of asynchronous OS events, buffer flushing, and termination callbacks.

## File streams

These rules follow `components/wide-io/lib/stdio.c`, `stdio-temporary.c`, and the public stdio headers. Related tests cover text translation, append/reopen behavior, buffered positions, update streams, and temporary files.

- Text output writes each LF as CR LF. Text input combines CR LF into LF and preserves a lone CR. Ctrl-Z has no special EOF meaning. No newline is added at the end, and spaces before newlines are retained. Binary mode performs none of these conversions and adds no padding to record boundaries.
- r, w, and a may be followed by one `+` and one `b` in either order. Other characters and duplicate modifiers are rejected. Without b, the mode is text. Ordinary opens allow Win32 read/write/delete sharing but can still fail due to filesystem conditions or other handles' sharing settings.
- w/w+ create or truncate on open. Ordinary update writes overwrite only the bytes actually written, without truncating later content; newline translation does not change this. Closing an empty ordinary file does not delete it; it can be reopened and read as EOF. Multiple fopen streams may open the same file, each with its own handle, position, and buffer, subject to available stream slots, permissions, and other handles' sharing settings.
- Filename bytes pass directly to CreateFileA. The runtime does not impose its own character restrictions or rewrite paths. Valid names, device names, case matching, and path resolution follow Windows and the target filesystem. The ANSI code page determines non-ASCII interpretation. FILENAME_MAX is a published capacity, not a promise that every shorter name can be opened.
- a starts at the end; a+ starts reading at the beginning. Append mode seeks to the end before every underlying write, so fseek cannot make later writes overwrite the middle. Seek-then-write is not guaranteed atomic between concurrent writers.
- stdin/stdout use GetConsoleMode: consoles are line-buffered, other handles fully buffered. stderr starts unbuffered; ordinary fopen streams start fully buffered. BUFSIZ is 4096; FOPEN_MAX is 32 including standard stream slots; FILENAME_MAX and L_tmpnam are 260.
- ungetc provides one byte of pushback. ftell reports the physical byte position adjusted for unread buffer and pushback bytes and pending writes. Negative values clamp to zero; values beyond LONG_MAX fail with ERANGE. fpos_t internally stores a 64-bit position. Text positions are not logical character counts.
- rename uses MoveFileExA with destination replacement allowed. remove distinguishes ordinary files and directories, calling DeleteFileA or RemoveDirectoryA; directories must meet system deletion conditions. Renaming or deleting open files depends on Win32 sharing permissions and the filesystem.
- Temporary files are placed in the directory returned by GetTempPathA. Names contain the process ID, an initialization-time timer value, and an incrementing sequence. Creation uses CREATE_NEW and retries with another name on collision. tmpfile opens binary update mode with delete-on-close. tmpnam briefly creates a candidate to validate it, closes and deletes it, and returns the name; it does not reserve that name for later use.

These are current-platform conventions. Different filesystems, nonseekable devices, asynchronous termination, and all failure paths require further testing. Internal FILE and fpos_t fields have implementation-reserved names and are not application interfaces.

## Formatting, diagnostics, and abnormal termination

The following conventions apply to this release.

- The fprintf family prints `%p` as a lowercase hexadecimal address with `0x`, without default padding to 16 digits; null is `0x0`. The fscanf family reads `%p` as hexadecimal, allowing `0x`/`0X` and requiring at least one digit, writing through `void **`. It accepts this implementation's printed pointer form. Sources are `runtime/lib/stdio-format.c` and `stdio-scan.c`; format/scan tests cover pointer conversion.
- In `%[`, an initial `]` may be a set member. Ascending forms such as `a-z` expand to inclusive byte ranges. Nonascending hyphens, and leading/trailing hyphens, are literal members. `^` complements the byte set. Matching uses byte values, not locale collation.
- Enabled assert writes `Assertion failed: expression, file filename, line number, function function-name` to stderr, then a newline, flushes stderr, and calls abort. The expression is stringified before macro-argument expansion. Disabled assert does not evaluate its argument. Sources are `base/include/assert.h` and `components/wide-io/lib/assert.c`; `base/checks/assert.ps1` checks exact output, flushing, and single evaluation.
- strerror maps 0, EDOM, ERANGE, EILSEQ, and EINVAL to `No error`, `Domain error`, `Range error`, `Illegal byte sequence`, and `Invalid argument`; other values yield `Unknown error`. perror saves errno, optionally writes a nonempty prefix and `: `, then the message and newline, and restores errno. It does not directly return Windows system error text.
- Invalid-stream checks in fgetpos and ftell set errno to 22. Underlying seek failures use the shared Win32 mapping: missing file/path → 2; access denied or sharing/lock conflict → 13; invalid handle → 9; out of memory → 12; invalid parameter → 22; disk full → 28; other errors → 5. ftell additionally sets ERANGE beyond LONG_MAX. See `__c1_file_failure`, `__c1_file_position`, and the public functions in `stdio.c`.
- abort ultimately calls ExitProcess directly through `_Exit`, without atexit callbacks or explicit C-stream flushing, except for actions performed by a SIGABRT handler. The OS closes handles on exit; tmpfile deletion relies on delete-on-close. Unwritten ordinary-file C buffers are not guaranteed to survive. This differs from normal exit's callback and stream-closing path.

These entries supplement C89 implementation-defined behavior documentation; see [C89 draft A.6.3.13](https://www.columbia.edu/~ng2573/c89/c89-draft.html#A.6.3.13).

## Integer library and random numbers

`random.c` starts with state 1; srand sets it to the supplied unsigned int. rand computes `state = state * 1103515245U + 12345U` using unsigned 32-bit arithmetic, then returns the low 15 bits after shifting right by 16, in 0..32767. Equal seeds reproduce the sequence; no srand call is equivalent to seed 1. No cryptographic properties are promised.

strtol/strtoul use C-locale whitespace, ASCII digits, and letters, supporting bases 2..36 and prefix-based selection for base zero. Scanning continues through all valid digits after overflow is detected, placing endptr after the complete digit sequence. Out-of-range results return the appropriate extreme and set ERANGE. Successful and no-conversion paths do not clear errno. Invalid bases are handled as an extension: return zero, set endptr to the input start, and set EINVAL. atoi/atol use decimal conversion.

No behavior beyond the standard's requirements is promised for unrepresentable abs, division, or atoi/atol results.

## Allocation, startup, and termination

`components/wide-io/lib/memory.c` uses the process heap. malloc(0) and zero-product calloc requests allocate one byte, returning a nonnull freeable pointer on success or null on failure. calloc checks multiplication overflow first and returns null on overflow; the heap interface zeros allocated storage. realloc(NULL,n) behaves as malloc(n), so realloc(NULL,0) follows malloc(0). For nonnull p, realloc(p,0) frees p and returns null. free(NULL) does nothing. Allocation wrappers do not additionally set errno.

The public `compile-program.ps1` driver selects main(int,char **) startup with `-MainArguments`; otherwise it uses main(void). `arguments.c` reads GetCommandLineA and allocates a writable argv array and strings with argv[argc] null. Program-name quoting is handled separately. Later arguments are split using space/tab, double-quote, and backslashes-before-quotes rules, with special handling for adjacent quotes and code-page lead bytes. There is no wildcard expansion, response-file reading, or lowercasing. Bytes originate from the Win32 interface; lossless arbitrary Unicode arguments are not promised.

`process.c` supports 32 pending atexit registrations, invoked in reverse registration order; registration failure returns nonzero. Normal main return passes its value to exit, which calls handlers, closes initialized streams, and converts status to unsigned int for ExitProcess. Command-line preparation failure returns 127. See Signal handling for abort.

memory-c89 tests cover zero-size allocations, calloc overflow, growth/shrinkage, retention on failure, independent allocations, NULL realloc, and alignment. Additional tests cover argument parsing, startup, and C89/C99 driver integration. Extreme exhaustion, all code pages, and all abnormal termination scenarios are not exhaustively verified.

## Time and environment

`base/include/time.h` defines time_t and clock_t as signed 64-bit long long, with CLOCKS_PER_SEC 10000000. `time.c` converts GetSystemTimeAsFileTime counts to whole seconds since 1970-01-01 UTC; difftime returns a difference in seconds. gmtime uses the proleptic Gregorian calendar and emits seconds 0..59, without representing leap seconds as additional seconds. Years beyond tm_year's range fail with ERANGE.

`clock.c` returns accumulated kernel plus user CPU time for the current process in 100-nanosecond units, not wall-clock or child-process time. GetProcessTimes failure returns `(clock_t)-1` with EINVAL; signed 64-bit result overflow sets ERANGE.

`localtime.c` and `dynamic-zone.c` use current Windows dynamic time-zone settings and query target-year rules when available. Time-zone queries, year ranges, or conversions may fail; time_t's representable range does not imply local-time conversion is available for every year. localtime and gmtime share static struct tm storage, overwritten by later calls. asctime uses a separate static character buffer; ctime calls localtime then asctime. With tm_isdst<0, mktime first constructs a candidate using the standard-time offset. If conversion back to wall time does not match, it tries the daylight offset, adopting it only on an exact match; otherwise it retains the first candidate and writes back the normalized result. Thus a typical one-hour fall overlap selects standard time, and a spring gap normalizes forward. Synthetic time-zone tests check this behavior. tm_isdst>0 selects the daylight offset and zero selects standard time, with results still written back according to time-zone rules. Dynamic-year rules iterate at most four times to obtain a consistent year; failure returns -1 with ERANGE. This does not exhaust every OS time zone or changes to settings during a call.

`environment.c` queries GetEnvironmentVariableA and returns a runtime-owned dynamically allocated string. The next successful query frees the previous successful result; callers must not modify, free, or retain it long-term. When a name points into the previous result, that storage remains until the new query completes. Missing or invalid names, query failures, and allocation failures return null. Encoding and name matching follow Win32; portability of all non-ASCII names/values across code pages is not guaranteed.

Variables come from the current process's Windows environment, usually supplied by its parent; the runtime has no separate fixed-name list. The C89 library provides no environment-modification function. If the host or program changes the current environment through Win32, later getenv calls read the updated environment, not a private startup snapshot.

`system.c` locates cmd.exe via GetSystemDirectoryA, runs commands with `/d /s /c`, and waits. COMSPEC does not select the interpreter. A null argument tests whether that path exists and is not a directory. On completion, the child exit code is converted to int; preparation, creation, waiting, or exit-code retrieval failures return -1. The child inherits the current environment, directory, and standard handles. Source programs must still construct commands according to command-interpreter syntax.

Time, CPU-clock, and system tests cover units, overflow, exit codes, quoting, environment, and standard I/O, not every time-zone or Windows configuration.

## Identifiers and translation resources

`intern` in `host/compiler.c` compares the first 63 identifier characters, case-sensitively. It still reads the remainder and retains the complete spelling location for diagnostics and other uses. Spellings with equal first 63 characters cannot denote distinct identifiers. This applies to internal and external names; external names are not truncated to six characters or case-folded.

Selected limits follow. These are shared internal-table capacities or recursion guards, not a promise that every maximum can be used simultaneously in one translation unit.

See [C89-LIMITS.md](C89-LIMITS.md) for capacities, pointer derivation chains, and shared budgets, and for the distinction between implementation limits and C89 minimum-capacity test coverage.

| Resource | Capacity or rejection boundary |
| --- | --- |
| intern identifier table | 2048 entries, including predefined names |
| qualified type table | 2048 entries |
| Declarator recursion | More than 64 levels rejected |
| One function parameter list | 127 parameters |
| Active switch case pool | 2048 records shared by nested switches |
| Preprocessor conditional nesting | 128 levels |
| Include path buffer | At most 1023 bytes plus terminator |
| Current include depth | Further inclusion rejected at depth 63 |

The combined limits program in `c89-supplemental-tests` uses both compiler generations to check C89 minimum capacities: 511 macros and file-scope objects, 127 local names/members/enumerators, 257 cases, 31 function and macro parameters, and 8 levels of include and conditional nesting. It does not test every maximum above or establish all higher C99 minimum capacities.

## Standards scope of accuracy claims

No uniform worst-case error bound is given for the entire mathematical library or all string/floating conversions; that global accuracy bound is explicitly unknown. Existing per-function tests and specific conversion conventions remain applicable. [WG14 N1256 §5.2.4.2.2 paragraph 5](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf) explicitly permits unknown accuracy to be stated for C99. This does not present C99 wording as original C89 text or waive domain, return-value, error-handling, or other mandatory semantics.

There is no library-wide guarantee of correct rounding or exhaustive coverage of floating inputs. See the technical documentation for each algorithm's accuracy conventions and test boundaries.

## Test scope

Test entry points and steps are in the [Walkthrough](../WALKTHROUGH.md). Results describe covered behavior, not all possible inputs.

Character, layout, pointer, rounding, mathematical-error, and signal probes are in `c89-implementation-tests/`. Other tests are supplied by the main toolchain and two further supplemental suites. Translation capacities are in [C89-LIMITS.md](C89-LIMITS.md).

These conventions and tests do not exhaust mathematical inputs, devices, or concurrency configurations. All C99 translation capacities and optional floating-point annexes are also outside the conformance claim.

## Header-path encoding

pp_search_header passes path bytes to CreateFileA using the active Windows ANSI code page. DBCS separator detection and directory joining skip trail bytes at character boundaries, avoiding treating a trail byte 0x5C as a separator. This differs from single-byte source mapping and does not promise arbitrary Unicode paths or compatibility under every active code page.

Quoted headers search the current included file's directory, working directory, nonempty semicolon-separated C1_INCLUDE directories, then the default include directory. Angle brackets omit the first two steps; absolute paths are opened directly. The source path buffer holds at most 1023 bytes plus terminator.

## Date/time macros and fallback

The compiler reads local date/time on the first expansion of __DATE__ or __TIME__ and caches it within the translation unit. Unavailable or unrepresentable data maps to `Jan  1 1970` and `00:00:00`. Valid years are 1..9999; month/day, leap-year, and clock fields are checked using Gregorian rules, and the callback buffer is zeroed first. Clock stubs test valid, invalid, and cached behavior without changing the system clock.

## Language modes and character encoding

The following conventions collect language-feature behavior for this release. Availability of a feature does not establish complete C99 conformance.

### Universal character names and function names

C99 mode supports UCNs in identifiers, character constants, and strings; C89 diagnoses them as extensions. Identifier stringification preserves spelling; name comparison uses decoded characters. Symbol tables and custom objects use canonical UTF-8, retaining 63 significant characters and at most 189 bytes; linker symbol names are limited to 255 bytes. Allowed ranges are in `bootstrap-c99/namespace/ucn/annex-d.txt`, following N1256 6.4.2.1, 6.4.3, and Annex D.

Narrow literals use a single-byte value range and wide literals a 16-bit range. A valid UCN outside the selected range maps to question mark (63). This does not automatically decode arbitrary UTF-8 source files or strings. Identifier UCNs receive no extra backslash during stringification, so later literal decoding interprets them.

`__func__` uses the complete function name at its definition, decodes UCNs, converts to the narrow execution character set, and creates a NUL-terminated const char array. Unrepresentable characters become question marks. The 63-character comparison limit does not truncate this array. Its execution encoding is independent of canonical UTF-8 object symbol names. This follows N1256 6.4.2.2.

Plain char is signed, so a single narrow character constant decoded to 128–255 has int value -128–-1. Sign extension occurs only for exactly one decoded unit; multicharacter packing, wide values, and stored string bytes are unchanged. References are C89 3.1.3.4 and N1256 6.4.4.4.

### Initialization, constant expressions, and return

| Context | Behavior |
| --- | --- |
| C89 automatic aggregate/union initializer lists | Parsed as constant expressions; automatic scalar initializers and unbraced record-copy initializers may still use general expressions. C99 retains runtime aggregate initialization |
| Commas in C89 constant expressions | Unevaluated comma expressions permitted by C99 receive an extension diagnostic. Ordinary runtime comma expressions and sizeof operands follow their own rules; comma expressions are not misclassified as C89 integer constant expressions |
| Bare return in a non-void function | Allowed in C89 mode; tests discard the returned value and check only side effects and early exit. C99 issues a constraint diagnostic. This does not assign a defined value to a missing return value |
| Returning an expression from a void function | Disallowed in both modes |

References include [C90 issue 150](https://www.open-std.org/jtc1/sc22/wg14/issues/c90/issue0150.html), C89 3.4, C99 6.6, [N1448](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1448.htm), and [N739](https://open-std.org/jtc1/sc22/wg14/www/docs/n739.htm). N1448's proposal is not treated as an adopted C99 corrigendum.

### Preprocessing constraints and variable-length arrays

Vertical tabs or form feeds between tokens of active preprocessing directives are diagnosed. Whitespace before directives, ordinary source whitespace, comments, literals, and included-file content follow their respective rules.

A preprocessed translation unit must contain an external declaration. Both C89/C99 modes diagnose an empty token stream before object output. Units containing only typedefs, forward tags, function prototypes, or extern object declarations remain valid. References are C89 3.7/2.1.1.3 and N1256 6.9/5.1.1.3.

C99 VLAs support prototype and runtime bounds, automatic array storage, saved sizeof/strides, function-parameter evaluation, scope cleanup, and constraints on variably modified type declarations. Tests and audits are in `bootstrap-c99/namespace/test-vla.ps1` and `audit-vla.ps1`. These features do not extend the limits stated elsewhere.

## Technical design and algorithms

See [DESIGN.md](../docs/DESIGN.md) for internal details such as initializer state preservation and public-header/runtime isolation, including the organization of complex arithmetic, type-generic mathematics, and the full mathematical runtime. Cube root, fused multiply-add, error functions, and Gamma algorithms, independent reference values, and accuracy boundaries are covered in the [Technical documentation guide](../docs/README.md).
