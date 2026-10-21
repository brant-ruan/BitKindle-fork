#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "files.c"
#include "merge.c"
#include "macho.c"

static struct lk_owned_object inputs[1024];
static unsigned char *objects[1024], *payloads[1024];
static unsigned long long lengths[1024];
static struct lk_section sections[1024];
static struct lk_symbol symbols[4096];
static struct lk_fixup fixups[16384];
static int initializers[1024];
static unsigned char startup[8192];

static int report(const char *message) {
    fputs("c1link-macho: ", stderr);
    fputs(message, stderr);
    fputc('\n', stderr);
    return 1;
}

int main(int argc, char **argv) {
    struct lk_object_counts counts;
    struct lk_macho_layout layout;
    unsigned long long entry_addr, target;
    int loaded = 0, i, entry_id = -1, init_count = 0, start_section = -1;
    const char *out_path, *entry_name;
    int cursor;

    if (argc < 4) {
        return report("usage: c1link-macho output entry-symbol input.c1o ...");
    }

    out_path = argv[1];
    entry_name = argv[2];

    for (i = 3; i < argc; i++) {
        if (loaded >= 1024)
            return report("too many input objects");
        if (lk_read_object_file(argv[i], inputs + loaded)) {
            fprintf(stderr, "c1link-macho: cannot read valid object file '%s'\n", argv[i]);
            return 1;
        }
        objects[loaded] = inputs[loaded].bytes;
        lengths[loaded] = inputs[loaded].length;
        loaded++;
    }

    /* 1. 合并所有目标文件中的节、符号与重定位 */
    if (lk_merge_objects(objects, lengths, loaded, sections, payloads, 1024,
                          symbols, 4096, fixups, 16384, &counts)) {
        return report("object merge failed");
    }

    /* 2. 符号绑定与入口查找 */
    if (lk_bind_symbols(symbols, counts.symbols)) {
        return report("symbol binding or resolution failed");
    }

    for (i = 0; i < counts.symbols; i++) {
        if (symbols[i].binding == LK_GLOBAL && lk_name_equal(symbols[i].name, entry_name)) {
            entry_id = i;
        }
        if (symbols[i].binding == LK_LOCAL && lk_name_equal(symbols[i].name, "$init")) {
            int s = symbols[i].section;
            if (init_count < 1024 && s >= 0 && s < counts.sections &&
                (sections[s].flags & LK_EXECUTE)) {
                initializers[init_count++] = i;
            }
        }
    }

    if (entry_id < 0) {
        fprintf(stderr, "c1link-macho: entry symbol '%s' has no global definition\n", entry_name);
        return 1;
    }

    /* 3. 创建 ARM64 启动桩节（Startup section） */
    start_section = counts.sections++;
    sections[start_section].alignment = 16;
    sections[start_section].file_alignment = 16;
    sections[start_section].flags = LK_READ | LK_EXECUTE;
    payloads[start_section] = startup;

    /*
     * ARM64 启动桩逻辑：
     * 1. 保存帧指针与链接寄存器：
     *    stp x29, x30, [sp, #-16]!  (0xa9bf7bfd)
     *    mov x29, sp                (0x910003fd)
     * 2. 依次调用各模块的 $init：
     *    bl target_init             (LK_ARM64_BRANCH26)
     * 3. 调用入口函数 (如 main)：
     *    bl entry                   (LK_ARM64_BRANCH26)
     * 4. 恢复并返回：
     *    ldp x29, x30, [sp], #16    (0xa8c17bfd)
     *    ret                        (0xd65f03c0)
     */
    cursor = 0;
    /* stp x29, x30, [sp, #-16]! */
    startup[cursor++] = 0xfd; startup[cursor++] = 0x7b; startup[cursor++] = 0xbf; startup[cursor++] = 0xa9;
    /* mov x29, sp */
    startup[cursor++] = 0xfd; startup[cursor++] = 0x03; startup[cursor++] = 0x00; startup[cursor++] = 0x91;

    /* 预留 initializers 的 bl 指令位置 */
    int init_offsets[1024];
    for (i = 0; i < init_count; i++) {
        init_offsets[i] = cursor;
        startup[cursor++] = 0x00; startup[cursor++] = 0x00; startup[cursor++] = 0x00; startup[cursor++] = 0x94; /* bl #0 */
    }

    /* 预留 entry 的 bl 指令位置 */
    int entry_call_offset = cursor;
    startup[cursor++] = 0x00; startup[cursor++] = 0x00; startup[cursor++] = 0x00; startup[cursor++] = 0x94; /* bl #0 */

    /* ldp x29, x30, [sp], #16 */
    startup[cursor++] = 0xfd; startup[cursor++] = 0x7b; startup[cursor++] = 0xc1; startup[cursor++] = 0xa8;
    /* ret */
    startup[cursor++] = 0xc0; startup[cursor++] = 0x03; startup[cursor++] = 0x5f; startup[cursor++] = 0xd6;

    sections[start_section].file_size = cursor;
    sections[start_section].memory_size = cursor;

    /* 4. 初步计算 Mach-O 虚拟地址与节区布局 */
    if (lk_macho_compute_layout(sections, counts.sections, 0, &layout)) {
        return report("macho layout computation failed");
    }

    (void)entry_addr;
    /* 启动桩的首地址即为 Mach-O 的入口地址 */
    entry_addr = sections[start_section].address;
    layout.entry_fileoff = sections[start_section].file_offset;

    /* 5. 解析并修补启动桩内部对 $init 和 entry 的跳转指令 (LK_ARM64_BRANCH26) */
    unsigned long long bases[1024];
    for (i = 0; i < counts.sections; i++)
        bases[i] = sections[i].address;

    /* 修补各个 $init 调用 */
    for (i = 0; i < init_count; i++) {
        if (lk_symbol_address(symbols, counts.symbols, initializers[i], bases, counts.sections, &target)) {
            return report("failed to resolve initializer address");
        }
        if (lk_relocate(startup, sections[start_section].file_size, init_offsets[i],
                        sections[start_section].address + init_offsets[i], target, 0,
                        layout.image_base, LK_ARM64_BRANCH26)) {
            return report("startup init call out of range");
        }
    }

    /* 修补 entry 调用 */
    if (lk_symbol_address(symbols, counts.symbols, entry_id, bases, counts.sections, &target)) {
        return report("failed to resolve entry address");
    }
    if (lk_relocate(startup, sections[start_section].file_size, entry_call_offset,
                    sections[start_section].address + entry_call_offset, target, 0,
                    layout.image_base, LK_ARM64_BRANCH26)) {
        return report("startup entry call out of range");
    }

    /* 6. 执行所有输入目标文件的重定位修补 */
    if (lk_apply_fixups(sections, payloads, counts.sections, symbols, counts.symbols,
                        fixups, counts.fixups, layout.image_base)) {
        return report("fixup relocation failed");
    }

    /* 7. 写出最终 Mach-O 文件并签名 */
    if (lk_write_macho(out_path, sections, payloads, counts.sections, &layout)) {
        return report("failed to write output Mach-O file");
    }

    printf("[SUCCESS] c1link-macho successfully created: %s\n", out_path);
    return 0;
}
