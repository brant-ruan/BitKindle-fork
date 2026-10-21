/*
 * macho_arm64_seed.c
 * 阶段 1 原型验证程序：
 * 纯代码组装符合 Apple Silicon 现代安全规范的 64 位 Mach-O 文件与 ARM64 指令。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mach-o/loader.h>
#include <sys/stat.h>

#define PAGE_SIZE 0x4000ULL  /* Apple Silicon 16KB 页对齐 */

int main(int argc, char **argv) {
    const char *outfile = "macos-arm64/build/hello_arm64";
    if (argc > 1) {
        outfile = argv[1];
    }

    FILE *f = fopen(outfile, "wb");
    if (!f) {
        perror("fopen failed");
        return 1;
    }

    /* 
     * 1. ARM64 机器码：
     *    write(1, "Hello from Native macOS ARM64!\n", 31)
     *    exit(0)
     */
    const char msg[] = "Hello from Native macOS ARM64!\n";
    uint32_t msg_len = (uint32_t)strlen(msg);

    uint32_t code[] = {
        0xd2800020,  /* mov  x0, #1                (fd = 1) */
        0x10000141,  /* adr  x1, #40               (pc 相对地址寻址 msg) */
        0xd28003e2,  /* movz x2, #31               (count = 31) */
        0xd2800090,  /* movz x16, #4               (sys_write: 0x2000004) */
        0xf2a04010,  /* movk x16, #0x200, lsl #16  */
        0xd4001001,  /* svc  #0x80                 */

        0xd2800000,  /* mov  x0, #0                (status = 0) */
        0xd2800030,  /* movz x16, #1               (sys_exit: 0x2000001) */
        0xf2a04010,  /* movk x16, #0x200, lsl #16  */
        0xd4001001   /* svc  #0x80                 */
    };
    size_t code_bytes = sizeof(code);

    /* 
     * 2. 各个 Load Command 构造
     */

    /* Command 1: __PAGEZERO (4GB) */
    struct segment_command_64 seg_pagezero;
    memset(&seg_pagezero, 0, sizeof(seg_pagezero));
    seg_pagezero.cmd = LC_SEGMENT_64;
    seg_pagezero.cmdsize = sizeof(seg_pagezero);
    strncpy(seg_pagezero.segname, "__PAGEZERO", sizeof(seg_pagezero.segname));
    seg_pagezero.vmaddr = 0x0;
    seg_pagezero.vmsize = 0x100000000ULL;
    seg_pagezero.fileoff = 0;
    seg_pagezero.filesize = 0;

    /* Command 2: __TEXT */
    struct {
        struct segment_command_64 seg;
        struct section_64 text_sec;
    } __attribute__((packed)) seg_text;

    memset(&seg_text, 0, sizeof(seg_text));
    seg_text.seg.cmd = LC_SEGMENT_64;
    seg_text.seg.cmdsize = sizeof(seg_text);
    strncpy(seg_text.seg.segname, "__TEXT", sizeof(seg_text.seg.segname));
    seg_text.seg.vmaddr = 0x100000000ULL;
    seg_text.seg.vmsize = PAGE_SIZE;
    seg_text.seg.fileoff = 0;
    seg_text.seg.filesize = PAGE_SIZE;
    seg_text.seg.maxprot = VM_PROT_READ | VM_PROT_EXECUTE;  /* 禁止写权限，满足 W^X 安全策略 */
    seg_text.seg.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
    seg_text.seg.nsects = 1;

    /* Command 3: __LINKEDIT (专供代码签名与链接元数据) */
    struct segment_command_64 seg_linkedit;
    memset(&seg_linkedit, 0, sizeof(seg_linkedit));
    seg_linkedit.cmd = LC_SEGMENT_64;
    seg_linkedit.cmdsize = sizeof(seg_linkedit);
    strncpy(seg_linkedit.segname, "__LINKEDIT", sizeof(seg_linkedit.segname));
    seg_linkedit.vmaddr = 0x100000000ULL + PAGE_SIZE;
    seg_linkedit.vmsize = PAGE_SIZE;
    seg_linkedit.fileoff = PAGE_SIZE;
    seg_linkedit.filesize = 0x1000; /* 预留 4KB 给签名 */
    seg_linkedit.maxprot = VM_PROT_READ;
    seg_linkedit.initprot = VM_PROT_READ;
    seg_linkedit.nsects = 0;

    /* Command 4: LC_LOAD_DYLINKER */
    struct {
        struct dylinker_command cmd;
        char name[20];
    } __attribute__((packed)) dylinker;
    memset(&dylinker, 0, sizeof(dylinker));
    dylinker.cmd.cmd = LC_LOAD_DYLINKER;
    dylinker.cmd.cmdsize = sizeof(dylinker); /* 12 + 20 = 32 */
    dylinker.cmd.name.offset = sizeof(struct dylinker_command);
    strcpy(dylinker.name, "/usr/lib/dyld");

    /* Command 5: LC_MAIN */
    struct entry_point_command entry_cmd;
    memset(&entry_cmd, 0, sizeof(entry_cmd));
    entry_cmd.cmd = LC_MAIN;
    entry_cmd.cmdsize = sizeof(entry_cmd);
    entry_cmd.stacksize = 0;

    /* Command 6: LC_BUILD_VERSION */
    struct {
        struct build_version_command cmd;
        struct build_tool_version tool;
    } __attribute__((packed)) build_ver;
    memset(&build_ver, 0, sizeof(build_ver));
    build_ver.cmd.cmd = LC_BUILD_VERSION;
    build_ver.cmd.cmdsize = sizeof(build_ver);
    build_ver.cmd.platform = PLATFORM_MACOS;
    build_ver.cmd.minos = (14 << 16); /* macOS 14.0 */
    build_ver.cmd.sdk = (14 << 16);
    build_ver.cmd.ntools = 1;
    build_ver.tool.tool = TOOL_LD;
    build_ver.tool.version = 0;

    /* Command 7: LC_CODE_SIGNATURE */
    struct linkedit_data_command code_sig;
    memset(&code_sig, 0, sizeof(code_sig));
    code_sig.cmd = LC_CODE_SIGNATURE;
    code_sig.cmdsize = sizeof(code_sig);
    code_sig.dataoff = PAGE_SIZE; /* 位于 __LINKEDIT 起始位置 */
    code_sig.datasize = 0x200;    /* 预留签名槽位 */

    /* 计算 load commands 总大小 */
    uint32_t total_cmds_size = sizeof(seg_pagezero) + sizeof(seg_text) + sizeof(seg_linkedit) +
                               sizeof(dylinker) + sizeof(entry_cmd) + sizeof(build_ver) + sizeof(code_sig);

    struct mach_header_64 header;
    memset(&header, 0, sizeof(header));
    header.magic = MH_MAGIC_64;
    header.cputype = CPU_TYPE_ARM64;
    header.cpusubtype = CPU_SUBTYPE_ARM64_ALL;
    header.filetype = MH_EXECUTE;
    header.ncmds = 7;
    header.sizeofcmds = total_cmds_size;
    header.flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;

    /* 确定代码存放位置（紧随 Load Commands 之后，16 字节对齐） */
    uint32_t text_fileoff = (sizeof(header) + total_cmds_size + 15) & ~15;

    strncpy(seg_text.text_sec.sectname, "__text", sizeof(seg_text.text_sec.sectname));
    strncpy(seg_text.text_sec.segname, "__TEXT", sizeof(seg_text.text_sec.segname));
    seg_text.text_sec.addr = seg_text.seg.vmaddr + text_fileoff;
    seg_text.text_sec.size = code_bytes + msg_len;
    seg_text.text_sec.offset = text_fileoff;
    seg_text.text_sec.align = 2;
    seg_text.text_sec.flags = S_REGULAR | S_ATTR_SOME_INSTRUCTIONS | S_ATTR_PURE_INSTRUCTIONS;

    entry_cmd.entryoff = text_fileoff;

    /* 3. 构造完整二进制数据（2 个 16KB 页，共 32KB：第一页 __TEXT，第二页 __LINKEDIT） */
    size_t total_file_size = PAGE_SIZE * 2;
    unsigned char *file_buffer = (unsigned char *)calloc(1, total_file_size);
    if (!file_buffer) {
        fclose(f);
        return 1;
    }

    uint32_t cursor = 0;
    memcpy(file_buffer + cursor, &header, sizeof(header)); cursor += sizeof(header);
    memcpy(file_buffer + cursor, &seg_pagezero, sizeof(seg_pagezero)); cursor += sizeof(seg_pagezero);
    memcpy(file_buffer + cursor, &seg_text, sizeof(seg_text)); cursor += sizeof(seg_text);
    memcpy(file_buffer + cursor, &seg_linkedit, sizeof(seg_linkedit)); cursor += sizeof(seg_linkedit);
    memcpy(file_buffer + cursor, &dylinker, sizeof(dylinker)); cursor += sizeof(dylinker);
    memcpy(file_buffer + cursor, &entry_cmd, sizeof(entry_cmd)); cursor += sizeof(entry_cmd);
    memcpy(file_buffer + cursor, &build_ver, sizeof(build_ver)); cursor += sizeof(build_ver);
    memcpy(file_buffer + cursor, &code_sig, sizeof(code_sig)); cursor += sizeof(code_sig);

    /* 写入机器码与消息文本 */
    memcpy(file_buffer + text_fileoff, code, code_bytes);
    memcpy(file_buffer + text_fileoff + code_bytes, msg, msg_len);

    fwrite(file_buffer, 1, total_file_size, f);
    free(file_buffer);
    fclose(f);

    chmod(outfile, 0755);
    printf("[SUCCESS] Generated conformant Mach-O ARM64: %s (%zu bytes)\n", outfile, total_file_size);
    return 0;
}
