/*
 * build_runtime.c
 * 阶段 4：生成 macOS 原生运行库 runtime.c1o 目标文件
 * 包含完全自包含的 ARM64 系统调用与标准 I/O 函数：
 * sys_write, sys_exit, putchar, print, puts, print_int
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../linker/writer.c"

int main(int argc, char **argv) {
    const char *outpath = "macos-arm64/build/runtime.c1o";
    if (argc > 1) outpath = argv[1];

    /* 
     * 72 条纯原生 ARM64 指令 (288 字节)，自包含内部跳转与分支：
     * 0x00: sys_write
     * 0x10: sys_exit
     * 0x20: putchar
     * 0x54: print
     * 0x9c: puts
     * 0xc0: print_int
     */
    uint32_t code[72] = {
        /* 0x00: sys_write */
        0xd2800090, 0xf2a04010, 0xd4001001, 0xd65f03c0,
        /* 0x10: sys_exit */
        0xd2800030, 0xf2a04010, 0xd4001001, 0xd65f03c0,
        /* 0x20: putchar */
        0xa9bf7bfd, 0x910003fd, 0xd10043ff, 0x390003e0,
        0x910003e1, 0xd2800020, 0xd2800022, 0xd2800090,
        0xf2a04010, 0xd4001001, 0x910003bf, 0xa8c17bfd,
        0xd65f03c0,
        /* 0x54: print */
        0xa9bf7bfd, 0x910003fd, 0xf81f0ff3,
        0xaa0003f3, 0xd2800002, 0x38626a63, 0x34000063,
        0x91000442, 0x17fffffd, 0xaa1303e1, 0xd2800020,
        0xd2800090, 0xf2a04010, 0xd4001001, 0xf84107f3,
        0x910003bf, 0xa8c17bfd, 0xd65f03c0,
        /* 0x9c: puts */
        0xa9bf7bfd,
        0x910003fd, 0x97ffffec, 0x52800140, 0x97ffffdd,
        0x52800000, 0x910003bf, 0xa8c17bfd, 0xd65f03c0,
        /* 0xc0: print_int */
        0xa9bf7bfd, 0x910003fd, 0xd10043ff, 0xb9000bf3,
        0x2a0003f3, 0x7100027f, 0x5400008a, 0x528005a0,
        0x97ffffd0, 0x4b1303f3, 0x71002a7f, 0x5400008b,
        0x52800141, 0x1ac10e60, 0x97fffff2, 0x52800141,
        0x1ac10e62, 0x1b01cc40, 0x1100c000, 0x97ffffc5,
        0xb9400bf3, 0x910003bf, 0xa8c17bfd, 0xd65f03c0
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

    /* 符号表导出 */
    struct lk_symbol syms[6];
    memset(syms, 0, sizeof(syms));

    syms[0].name = "sys_write"; syms[0].value = 0x00; syms[0].size = 16; syms[0].binding = LK_GLOBAL; syms[0].alignment = 4;
    syms[1].name = "sys_exit";  syms[1].value = 0x10; syms[1].size = 16; syms[1].binding = LK_GLOBAL; syms[1].alignment = 4;
    syms[2].name = "putchar";   syms[2].value = 0x20; syms[2].size = 52; syms[2].binding = LK_GLOBAL; syms[2].alignment = 4;
    syms[3].name = "print";     syms[3].value = 0x54; syms[3].size = 72; syms[3].binding = LK_GLOBAL; syms[3].alignment = 4;
    syms[4].name = "puts";      syms[4].value = 0x9c; syms[4].size = 36; syms[4].binding = LK_GLOBAL; syms[4].alignment = 4;
    syms[5].name = "print_int"; syms[5].value = 0xc0; syms[5].size = 96; syms[5].binding = LK_GLOBAL; syms[5].alignment = 4;

    unsigned char out_buf[65536];
    unsigned long long written = 0;
    int ret = lk_object_write(&sec, &payload, 1, syms, 6, NULL, 0, out_buf, sizeof(out_buf), &written);
    if (ret != 0) {
        fprintf(stderr, "lk_object_write runtime failed: %d\n", ret);
        return 1;
    }

    FILE *f = fopen(outpath, "wb");
    if (!f) {
        perror("fopen outpath failed");
        return 1;
    }
    fwrite(out_buf, 1, (size_t)written, f);
    fclose(f);

    printf("[SUCCESS] Generated Darwin runtime object: %s (%llu bytes, 6 exported symbols)\n",
           outpath, written);
    return 0;
}
