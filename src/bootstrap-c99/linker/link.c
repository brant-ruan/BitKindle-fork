#include "src/bootstrap-c99/linker/files.c"
#include "src/bootstrap-c99/linker/merge.c"
#include "src/bootstrap-c99/linker/pe.c"
#include "src/bootstrap-c99/linker/imports.c"
static struct lk_owned_object inputs[1024];
static unsigned char *objects[1024], *payloads[1024];
static unsigned long long lengths[1024], bases[1024];
static struct lk_section sections[1024];
static struct lk_symbol symbols[4096];
static struct lk_fixup fixups[16384];
static int initializers[1024];
static unsigned char startup[8192];
static struct lk_import imports[256];
static const char *import_symbols[256];
static unsigned int import_slots[256];
static unsigned char import_stubs[1536];

static int report(const char *message) {
    fputs("c1link: ", stderr);
    fputs(message, stderr);
    fputc('\n', stderr);
    return 1;
}

int main(int argc, char **argv) {
    struct lk_object_counts counts;
    struct lk_import_layout import_layout;
    unsigned long long total, image_size, end, entry, written, target;
    unsigned char *output = 0, *import_data = 0;
    FILE *file = 0;
    int loaded = 0, i, n, common = 0, entry_id = -1, status = 1, bad, init_count = 0,
        start_section = -1, cursor, s;
    int import_count = 0, stub_section = -1, import_section = -1, j;
    unsigned int ordinal, import_rva;
    int subsystem = 3, subsystem_set = 0;
    if (argc < 4)
        return report(
            "usage: c1link output.exe entry-symbol input.c1o ... [--subsystem console|windows] [--import symbol DLL export|#ordinal]");
    for (i = 3; i < argc; i++) {
        if (lk_name_equal(argv[i], "--subsystem")) {
            if (subsystem_set || i + 1 >= argc)
                return report("missing or duplicate --subsystem");
            subsystem_set = 1;
            i++;
            if (lk_name_equal(argv[i], "console"))
                subsystem = 3;
            else if (lk_name_equal(argv[i], "windows"))
                subsystem = 2;
            else
                return report("--subsystem must be console or windows");
            continue;
        }
        if (lk_name_equal(argv[i], "--import")) {
            if (i + 3 >= argc || import_count == 255) {
                report("invalid import arguments");
                goto done;
            }
            import_symbols[import_count] = argv[++i];
            imports[import_count].dll = argv[++i];
            imports[import_count].name = argv[++i];
            if (argv[i][0] == '#') {
                ordinal = 0;
                j = 1;
                while (argv[i][j]) {
                    if (argv[i][j] < '0' || argv[i][j] > '9' || ordinal > 6553) {
                        report("invalid import ordinal");
                        goto done;
                    }
                    ordinal = ordinal * 10 + (unsigned int)(argv[i][j++] - '0');
                }
                if (!ordinal || ordinal > 65535) {
                    report("invalid import ordinal");
                    goto done;
                }
                imports[import_count].ordinal = ordinal;
                imports[import_count].name = 0;
            }
            import_count++;
            continue;
        }
        if (loaded == 1024) {
            report("too many input objects");
            goto done;
        }
        if (lk_read_object_file(argv[i], inputs + loaded)) {
            report("cannot read valid object");
            goto done;
        }
        objects[loaded] = inputs[loaded].bytes;
        lengths[loaded] = inputs[loaded].length;
        loaded++;
    }
    n = loaded;
    if (import_count) {
        import_symbols[import_count] = "$exit";
        imports[import_count].dll = "KERNEL32.dll";
        imports[import_count].name = "ExitProcess";
        imports[import_count].ordinal = 0;
        import_count++;
    }
    if (lk_merge_objects(objects, lengths, n, sections, payloads, 1024, symbols, 4096, fixups,
                         16384, &counts)) {
        report("object merge failed");
        goto done;
    }
    if (import_count) {
        if (counts.sections > 94 || counts.symbols > 4096 - import_count ||
            lk_build_imports(imports, import_count, 0, 0, 0, import_slots, &import_layout)) {
            report("invalid imports or import capacity exceeded");
            goto done;
        }
        import_data = malloc(import_layout.size);
        if (!import_data) {
            report("out of memory");
            goto done;
        }
        stub_section = counts.sections++;
        import_section = counts.sections++;
        sections[stub_section].memory_size = import_count * 6;
        sections[stub_section].file_size = import_count * 6;
        sections[stub_section].alignment = 16;
        sections[stub_section].file_alignment = 1;
        sections[stub_section].flags = LK_READ | LK_EXECUTE;
        sections[import_section].memory_size = import_layout.size;
        sections[import_section].file_size = import_layout.size;
        sections[import_section].alignment = 8;
        sections[import_section].file_alignment = 1;
        sections[import_section].flags = LK_READ | LK_WRITE;
        payloads[stub_section] = import_stubs;
        payloads[import_section] = import_data;
        for (i = 0; i < import_count; i++) {
            s = counts.symbols++;
            symbols[s].name = import_symbols[i];
            symbols[s].module = n;
            symbols[s].binding = LK_GLOBAL;
            symbols[s].section = stub_section;
            symbols[s].value = i * 6;
            symbols[s].size = 6;
            symbols[s].alignment = 1;
        }
    }
    if (lk_bind_symbols(symbols, counts.symbols)) {
        report("invalid or duplicate symbol definition");
        goto done;
    }
    for (i = 0; i < counts.symbols; i++) {
        if (symbols[i].binding == LK_COMMON && symbols[i].selected == i)
            common = 1;
        if (symbols[i].binding == LK_GLOBAL && lk_name_equal(symbols[i].name, argv[2]))
            entry_id = i;
        if (symbols[i].binding == LK_LOCAL && lk_name_equal(symbols[i].name, "$init")) {
            s = symbols[i].section;
            if (init_count == 1024 || s < 0 || s >= counts.sections ||
                !(sections[s].flags & LK_EXECUTE) || symbols[i].value >= sections[s].file_size) {
                report("invalid initializer entry");
                goto done;
            }
            initializers[init_count++] = i;
        }
    }
    if (entry_id < 0) {
        report("entry symbol has no global definition");
        goto done;
    }
    s = symbols[entry_id].section;
    if (s < 0 || s >= counts.sections || !(sections[s].flags & LK_EXECUTE) ||
        symbols[entry_id].value >= sections[s].file_size) {
        report("entry symbol is not initialized executable code");
        goto done;
    }
    if (counts.sections + common + (init_count != 0 || import_count != 0) > 96) {
        report("PE section capacity exceeded");
        goto done;
    }
    if (init_count || import_count) {
        start_section = counts.sections++;
        i = start_section;
        sections[i].memory_size = (import_count ? 19 : 14) + 5 * init_count;
        sections[i].file_size = sections[i].memory_size;
        sections[i].alignment = 16;
        sections[i].file_alignment = 1;
        sections[i].flags = LK_READ | LK_EXECUTE;
        payloads[i] = startup;
    }
    if (common) {
        i = counts.sections++;
        sections[i].memory_size = 1;
        sections[i].file_size = 0;
        sections[i].alignment = 4096;
        sections[i].file_alignment = 512;
        sections[i].flags = LK_READ | LK_WRITE;
        payloads[i] = 0;
    }
    if (lk_pe_layout(sections, counts.sections, &total, &image_size)) {
        report("unsupported PE section layout");
        goto done;
    }
    if (common) {
        i = counts.sections - 1;
        if (lk_layout_common(symbols, counts.symbols, sections[i].address, 0x803f0000ULL, &end)) {
            report("common allocation failed");
            goto done;
        }
        sections[i].memory_size = end - sections[i].address;
        if (!sections[i].memory_size)
            sections[i].memory_size = 1;
        if (lk_pe_layout(sections, counts.sections, &total, &image_size)) {
            report("common section exceeds image limits");
            goto done;
        }
    }
    for (i = 0; i < counts.sections; i++)
        bases[i] = sections[i].address;
    if (import_count) {
        import_rva = (unsigned int)(sections[import_section].address - 0x400000);
        if (lk_build_imports(imports, import_count, import_rva, import_data,
                             sections[import_section].file_size, import_slots, &import_layout)) {
            report("import table layout failed");
            goto done;
        }
        for (i = 0; i < import_count; i++) {
            cursor = i * 6;
            import_stubs[cursor] = 0xff;
            import_stubs[cursor + 1] = 0x25;
            if (lk_relocate(import_stubs, import_count * 6, cursor + 2,
                            sections[stub_section].address + cursor + 2,
                            0x400000ULL + import_slots[i], 0, 0x400000, LK_REL32)) {
                report("import stub out of range");
                goto done;
            }
        }
    }
    if (lk_symbol_address(symbols, counts.symbols, entry_id, bases, counts.sections, &entry)) {
        report("entry address cannot be resolved");
        goto done;
    }
    if (lk_apply_fixups(sections, payloads, counts.sections, symbols, counts.symbols, fixups,
                        counts.fixups, 0x400000)) {
        report("unresolved symbol or invalid relocation");
        goto done;
    }
    if (init_count || import_count) {
        startup[0] = 0x48;
        startup[1] = 0x83;
        startup[2] = 0xec;
        startup[3] = 0x28;
        cursor = 4;
        for (i = 0; i <= init_count; i++) {
            target = entry;
            if (i < init_count && lk_symbol_address(symbols, counts.symbols, initializers[i], bases,
                                                    counts.sections, &target)) {
                report("initializer address cannot be resolved");
                goto done;
            }
            startup[cursor++] = 0xe8;
            if (lk_relocate(startup, sections[start_section].file_size, cursor,
                            sections[start_section].address + cursor, target, 0, 0x400000,
                            LK_REL32)) {
                report("startup call out of range");
                goto done;
            }
            cursor += 4;
        }
        if (import_count) {
            startup[cursor++] = 0x89;
            startup[cursor++] = 0xc1;
            startup[cursor++] = 0xff;
            startup[cursor++] = 0x15;
            if (lk_relocate(startup, sections[start_section].file_size, cursor,
                            sections[start_section].address + cursor,
                            0x400000ULL + import_slots[import_count - 1], 0, 0x400000, LK_REL32)) {
                report("exit call out of range");
                goto done;
            }
            cursor += 4;
            startup[cursor++] = 0xeb;
            startup[cursor++] = 0xfe;
        } else {
            startup[cursor++] = 0x48;
            startup[cursor++] = 0x83;
            startup[cursor++] = 0xc4;
            startup[cursor++] = 0x28;
            startup[cursor++] = 0xc3;
        }
        entry = sections[start_section].address;
    }
    output = malloc((size_t)total);
    if (!output) {
        report("out of memory");
        goto done;
    }
    if (lk_pe_write(sections, payloads, counts.sections, entry, output, total, &written)) {
        report("invalid PE image or entry point");
        goto done;
    }
    /* PE32+ Subsystem is a 16-bit field; preserve adjacent DllCharacteristics. */
    output[0xdc] = (unsigned char)subsystem;
    output[0xdd] = 0;
    if (import_count) {
        lk_object_put32(output + 0x110, import_rva);
        lk_object_put32(output + 0x114, import_layout.descriptor_size);
        lk_object_put32(output + 0x168, import_rva + import_layout.iat_offset);
        lk_object_put32(output + 0x16c, import_layout.iat_size);
    }
    file = fopen(argv[1], "wb");
    if (!file) {
        report("cannot open output");
        goto done;
    }
    bad = fwrite(output, 1, (size_t)written, file) != (size_t)written;
    if (fclose(file))
        bad = 1;
    file = 0;
    if (bad) {
        report("output write failed");
        goto done;
    }
    status = 0;
done:
    if (file)
        fclose(file);
    free(output);
    free(import_data);
    for (i = 0; i < loaded; i++)
        free(inputs[i].bytes);
    return status;
}

#include "src/bootstrap-c99/base/lib/memory.c"
#include "src/bootstrap-c99/base/lib/string.c"
#include "src/bootstrap-c99/base/lib/errno.c"
#include "src/bootstrap-c99/base/lib/stdio.c"
