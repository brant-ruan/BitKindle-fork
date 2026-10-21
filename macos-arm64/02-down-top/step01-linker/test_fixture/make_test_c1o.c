/*
 * make_test_c1o.c
 * 生成一个标准的 ARM64 .c1o 目标文件，用于验证 c1link-macho 的链接功能。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/writer.c"

int main(int argc, char **argv) {
    const char *outpath = "macos-arm64/build/test_func.c1o";
    if (argc > 1) outpath = argv[1];

    /* ARM64 机器码:
     * int my_entry(void) {
     *     return 42;
     * }
     * 指令:
     * mov  w0, #42   (0x52800540)
     * ret            (0xd65f03c0)
     */
    uint32_t code[] = {
        0x52800540,  /* mov w0, #42 */
        0xd65f03c0   /* ret */
    };

    struct lk_section sec;
    memset(&sec, 0, sizeof(sec));
    sec.memory_size = sizeof(code);
    sec.file_size = sizeof(code);
    sec.alignment = 16;
    sec.file_alignment = 16;
    sec.flags = LK_READ | LK_EXECUTE;

    unsigned char *payload = (unsigned char *)code;

    struct lk_symbol sym;
    memset(&sym, 0, sizeof(sym));
    sym.name = "my_entry";
    sym.binding = LK_GLOBAL;
    sym.section = 0;
    sym.value = 0;
    sym.size = sizeof(code);
    sym.alignment = 4;

    unsigned char out_buf[4096];
    unsigned long long written = 0;

    int ret = lk_object_write(&sec, &payload, 1, &sym, 1, NULL, 0, out_buf, sizeof(out_buf), &written);
    if (ret != 0) {
        fprintf(stderr, "lk_object_write failed with error: %d\n", ret);
        return 1;
    }

    FILE *f = fopen(outpath, "wb");
    if (!f) {
        perror("fopen failed");
        return 1;
    }
    fwrite(out_buf, 1, (size_t)written, f);
    fclose(f);

    printf("[SUCCESS] Generated test ARM64 .c1o object file: %s (%llu bytes)\n", outpath, written);
    return 0;
}
