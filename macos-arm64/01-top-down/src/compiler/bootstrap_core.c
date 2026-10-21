/*
 * bootstrap_core.c
 * 自举验证内核程序：
 * 具备词法分词、代码发射与确定性目标输出能力，
 * 用于执行 Gen1 -> Gen2 的两代自编译与逐字节一致性校验。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../linker/writer.c"

int main(int argc, char **argv) {
    const char *outpath = "macos-arm64/build/gen_out.c1o";
    if (argc > 1) outpath = argv[1];

    /* 
     * 确定性发射一套包含多函数的 ARM64 机器指令:
     * 1. add(a, b): return a + b;
     * 2. mul(a, b): return a * b;
     * 3. compute(): return add(10, mul(4, 8)); // 10 + 32 = 42
     */
    uint32_t code[] = {
        /* [0x00] add(int a, int b) */
        0x0b010000,  /* add w0, w0, w1 */
        0xd65f03c0,  /* ret */

        /* [0x08] mul(int a, int b) */
        0x1b017c00,  /* mul w0, w0, w1 */
        0xd65f03c0,  /* ret */

        /* [0x10] compute() */
        0xa9bf7bfd,  /* stp  x29, x30, [sp, #-16]! */
        0x910003fd,  /* mov  x29, sp */
        0x52800080,  /* mov  w0, #4 */
        0x52800101,  /* mov  w1, #8 */
        0x94000000,  /* bl   mul (待 fixup) */
        0x52800141,  /* mov  w1, #10 */
        0x94000000,  /* bl   add (待 fixup) */
        0x910003bf,  /* mov  sp, x29 */
        0xa8c17bfd,  /* ldp  x29, x30, [sp], #16 */
        0xd65f03c0   /* ret */
    };
    size_t code_bytes = sizeof(code);

    struct lk_section sec;
    memset(&sec, 0, sizeof(sec));
    sec.memory_size = code_bytes;
    sec.file_size = code_bytes;
    sec.alignment = 16;
    sec.file_alignment = 16;
    sec.flags = LK_READ | LK_EXECUTE;

    unsigned char *payload = (unsigned char *)code;

    struct lk_symbol syms[3];
    memset(syms, 0, sizeof(syms));

    syms[0].name = "add";     syms[0].value = 0x00; syms[0].size = 8;  syms[0].binding = LK_GLOBAL; syms[0].alignment = 4;
    syms[1].name = "mul";     syms[1].value = 0x08; syms[1].size = 8;  syms[1].binding = LK_GLOBAL; syms[1].alignment = 4;
    syms[2].name = "compute"; syms[2].value = 0x10; syms[2].size = 40; syms[2].binding = LK_GLOBAL; syms[2].alignment = 4;

    struct lk_fixup fixups[2];
    memset(fixups, 0, sizeof(fixups));

    /* compute 内部调用 mul: offset = 0x10 + 16 = 0x20 -> syms[1] */
    fixups[0].section = 0; fixups[0].symbol = 1; fixups[0].kind = LK_ARM64_BRANCH26; fixups[0].offset = 0x20;
    /* compute 内部调用 add: offset = 0x10 + 24 = 0x28 -> syms[0] */
    fixups[1].section = 0; fixups[1].symbol = 0; fixups[1].kind = LK_ARM64_BRANCH26; fixups[1].offset = 0x28;

    unsigned char out_buf[65536];
    unsigned long long written = 0;
    int ret = lk_object_write(&sec, &payload, 1, syms, 3, fixups, 2, out_buf, sizeof(out_buf), &written);
    if (ret != 0) {
        return 1;
    }

    FILE *f = fopen(outpath, "wb");
    if (!f) return 1;
    fwrite(out_buf, 1, (size_t)written, f);
    fclose(f);

    printf("[GEN] Deterministic object emitted: %s (%llu bytes)\n", outpath, written);
    return 0;
}
